#include "giga-status.h"
/*
 * loader.c: image loader and menu creator
 */


#ifdef SIMULATE
# include "mocktypes.h"
#else
# include <avr/pgmspace.h>
# include <util/delay.h>
#endif

#include <lib/pff.h>
#include <jtag/micro.h>
#include "loader.h"
#include "wire.h"


/*
 * Memory layout of generated menu:
 *
 * >6000:          GPL header
 * >6010:          general menu and browser code
 *                 - end of fixed program -
 * >6A00:          TEXT 'FILENAME'            (8 chars)
 *                 DATA <image type>          (>00 GROM, >FF ROM)
 *                 DATA <start address>
 *                 TEXT 'ENTRY NAME'          (20 chars)
 *         --or--  TEXT '/ FOLDER NAME'
 * TOTAL:          171 x 32 = >1560 bytes
 *
 * Image types: >00--  GROM
 *              >ff--  ROM
 *       where: >---0  normal   --->     >--1-  ROM mode
 *              >ff01  folder            >--2-  GROM mode
 *              >ff02  Help Viewer       >--3-  ROM/GROM moder
 *              >--03  dump image
 *              >ff04  E/A5 program (not used)
 *              (no type for Reload)
 */

#define BYTES_TO_SEND       12   // "FILENAMEttaa"
#define BROWSER_ENTRY_SIZE  32
#define ENTRY_NAME_LEN      20
#define MAX_ENTRIES         (9 * 19)

#define BANK_SIZE            0x2000
#define MENU_PROG_SIZE       0x0A00
#define FG99_RESTART         0x6020  // menu restart address (SADDR)


/*
 * globals and statics
 */

// SD card data structures
static FATFS sd_fs;
static DIR sd_dp;
static FILINFO sd_fno;
static UINT bytes_read;

// universal buffer
static uint8_t path[128];  // 12+ levels
static uint8_t path_len;
uint8_t selection[BYTES_TO_SEND + 5];  // + ".BIN\0"
uint8_t buffer[1024];

// global variables
static uint8_t total_entries;
static uint8_t subprograms;
static uint32_t image_size;
static uint8_t rom_mask, grom_mask;
static uint8_t rom_dump_bank, grom_dump_bank;

// pre-compiled TI programs
#include "menu.c"
#include "grom.c"
#include "help.c"

// internal functions
static uint8_t genFolderEntry(const uint8_t *);
static uint8_t genProgEntries(uint8_t *);
static uint8_t genAutostartEntry(uint8_t *,uint16_t);
static void genMenuCode(void);
static void genMenuGROM(void);
static void saveFile(void);
static uint32_t loadFile(uint8_t *,uint8_t);
static uint8_t *buildPath(char *);
static void dumpBank(uint8_t, uint8_t);
static void jtag(void) __attribute__((noinline));
static void genFileEntry(uint8_t *, uint16_t, uint16_t, uint8_t);

// bootloader
static void (*bootloader)(void) = (void *)0x3800;  // word address for m328


/*
 * initialize cart
 */

__attribute__((noinline)) void setup()
{
  init();

  path[0] = 0;
  path_len = 0;

  // open SD card
  _delay_ms(125);  // Startup-only sequencing trial; cartridge remains offline.
  if (pf_mount(&sd_fs))
    flash_error(1);
}


/*
 * build menu by scanning directory for dirs and *.BIN files
 */

#include "loader-giga.inc"
#include "compilation-session.inc"


__attribute__((noinline)) uint8_t genMenu()
{
  uint8_t total_images = 0, has_folder = 0;
  has_giga = 0;
  total_entries = 0;
  subprograms = 0;

  // disable cart
  led(1);
  writeBegin();
  setConfig(0);  // AVR reset does not reset the CPLD personality.

  // write GROM wrapper
  genMenuGROM();

  // build ROM code
  writeSize(0x00);

  // open current folder
  path[path_len] = 0;
  if (pf_opendir(&sd_dp, (const char *const)path))
    flash_error(2);

  // write fixed menu and browser code
  genMenuCode();

  // folder up entry
  if (path_len > 0)
    total_entries += genFolderEntry((const uint8_t *)"..");

  // entries for SD card
  while (total_entries < MAX_ENTRIES) {
    // get next item in directory
    if (pf_readdir(&sd_dp, &sd_fno))
      flash_error(2);
    if (sd_fno.fname[0] == 0)  // end of dir
      break;
    if (sd_fno.fattrib & (AM_HID | AM_SYS))
      continue;  // skip hidden and system files

    uint8_t added = (sd_fno.fattrib & AM_DIR) ?
      genFolderEntry((const uint8_t *)sd_fno.fname) :
      genProgEntries((uint8_t *)sd_fno.fname);
    total_entries += added;

    if (added) {
      ++total_images;
      if(sd_fno.fattrib & AM_DIR) has_folder=1;
    }
  }

  // mark end of entries
  for (uint8_t i = BROWSER_ENTRY_SIZE; i > 0; --i)
    writeByte(0);

  // led off
  writeEnd();
  led(0);

  // if only one image/subprograms found in "/", load file directly
  return total_images + subprograms == 1 && path_len == 0 && !has_giga && !has_folder;
}

static uint8_t genFolderEntry(const uint8_t *fn)
{
  uint8_t i;

  if (path_len + 13 > sizeof(path) - 1)
    return 0;  // path too deep

  const uint8_t *p = fn;
  while (*p)
    if (*p++ == '.')
      if (*p++ != '.')
        return 0;  // skip dirs with '.', but keep ".."

  p = fn;
  for (i = 8; i > 0; --i)
    writeByte(*p ? *p++ : 0);  // dirname has no extension

  writeByte(0xff);  // folder type
  writeByte(SEL_FOLDER);

  writeByte(FG99_RESTART >> 8);  // start addr of FinalGROM
  writeByte(FG99_RESTART & 0xff);

  writeByte('/');  // folder marker
  writeByte(' ');
  for (p = fn, i = ENTRY_NAME_LEN - 2; *p; --i)
    writeByte(*p++);  // filename w/o extension
  while (i-- > 0)
    writeByte(' ');

  /* The caller counts each folder exactly once. */

  return 1;
}


static uint8_t genProgEntries(uint8_t *fn)
{
  uint8_t i;
  uint8_t *p;
  uint16_t next;

  // autostart cartridges and start addresses beyond >7FFF
  // ruined this code! ;-)

  uint8_t *q = fn;
  while (*++q);  // end of filename

  if (!path_len && !strcmp((char *)fn,"UPDATE.AVR")) { cli(); SPCR=0; bootloader(); }
  if (!path_len && !strcmp((char *)fn,"UPDATE.PLD")) jtag();
  if (q-fn>=5 && !memcmp(q-4,".GC",3) && (q[-1]=='S' || q[-1]=='R'))
    return genGigaEntry(fn);

  // check if file ends in ".BIN"
  if (q - fn < 5 ||
      *--q != 'N' || *--q != 'I' || *--q != 'B' || *--q != '.')
    return 0;
  --q;  // q at last char of filename w/o extension

  // check image size
  if (sd_fno.fsize > 1024L * 1024L)  // 1 MB max.
    return 0;

  if (*q=='G' && sd_fno.fsize>40960UL) return 0;

  // skip secondary bank D
  if (*q == 'D' && sd_fno.fsize == 8192)
    return 0;

  // set image type
  uint16_t img_type = *q == 'G' ? 0x0000 : 0xff00;

  // read metadata
  if (pf_fnoopen(&sd_fno))
    flash_error(3);

  // search for metadata
  uint16_t pos = 0x0000;
  while (1) {
    if (pf_read(buffer, 24, &bytes_read))
      return 0;  // bad file
    if (bytes_read < 24)
      return 0;  // corrupt image
    if (*q != 'G' && (buffer[0] != 0xaa || buffer[6] < 0x60 || buffer[6] >= 0x80))
      return 0;  // invalid ROM image
    if (*q == 'G' && buffer[0] == 0xaa) {  // GROM image
      if (buffer[1] >= 0x80)
        return genAutostartEntry(fn,pos);  // autostart image
    }
    next = ((uint16_t)buffer[6] << 8) + buffer[7];  // first menu entry
    if (*q != 'G' ||  // valid ROM image
        (buffer[0] == 0xaa && next >= 0x6000))  // valid GROM image
      break;
    if ((uint32_t)pos + 0x2000 >= sd_fno.fsize) return 0;
    pos += 0x2000;
    if(pf_lseek(pos)) return 0;  // check higher GROM for start menu
  }

  // check for subprograms
  if (*q == 'G' && buffer[10] >= 0x60)
    ++subprograms;

  // get mode configuration
  switch (buffer[3]) {
  case 'G': img_type |= MODE_GRAM;  break;
  case 'R': img_type |= MODE_RAM;   break;
  case 'X': img_type |= MODE_RAM | MODE_GRAM;  break;
  }

  // read all entries from metadata
  uint8_t entries_sent = 0;
  while (next) {
    if (total_entries == 0)
      saveFile();  // might be needed for immediate load

    uint16_t offset = next - 0x6000;
    if (offset > sd_fno.fsize)
      break;  // corrupt image
    if (total_entries + entries_sent >= MAX_ENTRIES)
      break;  // too many entries

    // analyze metadata
    if (pf_lseek(offset))
      break;  // corrupt
    if(pf_read(buffer, 6 + ENTRY_NAME_LEN, &bytes_read) || bytes_read<5) break;
    p = buffer;

    next = (*p++ << 8);  // save next entry
    next += *p++;

    // write browser entry
    const uint8_t *r = fn;
    for (i = 8; i > 0; --i)
      writeByte(*r != '.' ? *r++ : 0);  // filename w/o extension

    writeByte(img_type >> 8);  // ROM or GROM image
    writeByte(img_type & 0xff);

    writeByte(*p++);  // start address in image
    writeByte(*p++);

    uint8_t len = *p++;  // length and name
    if(len>bytes_read-5) len=bytes_read-5;
    if (len>=2 && *p == '"' && *(p + len - 1) == '"') {  // remove "'s
      ++p;
      len -= 2;
    }
    for (i = 0; i < ENTRY_NAME_LEN; ++i)  // always pad name
    {
      uint8_t c=i<len?*p++:' ';
      writeByte(c>='a' && c<='z'?c-32:c);
    }

    ++entries_sent;
  }

  return entries_sent;
}


// generate menu entry without image entry
static uint8_t genAutostartEntry(uint8_t *fn,uint16_t pos)
{
  // GROM or GRAM?
  uint16_t type = buffer[3] == 'G' ? 0xff00 | MODE_GRAM : 0x0000;

  // save address
  saveFile();  // needed if single image

  // check autostart addresses for data
  uint8_t *q = buffer + 16;  // >6010 module autostart
  if (!*q)
    q += 3;  // >6013 menu autostart
  if (*q == 0)
    return 0;  // no start address found
  uint16_t start = 0x6000 + pos + (q - buffer);

  // generate entry
  genFileEntry(fn, type, start, '*');

  return 1;
}


// generate entry based on filename
static void genFileEntry(uint8_t *fn, uint16_t type, uint16_t addr, uint8_t suffix)
{
  uint8_t i;

  // filename
  uint8_t *p = fn;
  for (i = 8; i > 0; --i)
    writeByte(*p != '.' ? *p++ : 0);

  // type and start address
  writeByte(type >> 8);  // GROM
  writeByte(type & 0xff);
  writeByte(addr >> 8);  // autostart address
  writeByte(addr & 0xff);

  // entry name
  i = ENTRY_NAME_LEN;
  for (p = fn; *p != '.'; --i)
    writeByte(*p++);
  writeByte('*');
  while (i-- > 1)
    writeByte(' ');
}


// generate fixed part of menu program
static void genMenuCode()
{
  /* Exact generated menu, kept on SD to restore ordinary cartridge support.
   * Validate before receiveBegin publishes the cartridge to the console. */
  if (pf_open("FGMENU.BIN") || sd_fs.fsize!=2310) flash_error(3);
  uint16_t pos=0;
  uint32_t sum=0xffffffffUL;
  while (pos<2310) {
    UINT got, count=2310-pos;
    if(count>512) count=512;
    if(pf_read(buffer,count,&got) || got!=count) flash_error(3);
    sum=crc(sum,buffer,got);
    for(UINT n=0;n<got;++n,++pos) {
      uint8_t b=buffer[n];
      if(pos>=GIGA_STATUS_OFFSET && pos<GIGA_STATUS_OFFSET+30) {
        uint8_t k=pos-GIGA_STATUS_OFFSET;
        b=k<strlen(giga_status)?giga_status[k]:' ';
      } else if(pos>=GIGA_DETAIL_OFFSET && pos<GIGA_DETAIL_OFFSET+30) {
        uint8_t k=pos-GIGA_DETAIL_OFFSET;
        b=k<strlen(giga_detail)?giga_detail[k]:' ';
      }
      writeByte(b);
    }
  }
  if((sum^0xffffffffUL)!=0x54f4b98aUL) flash_error(3);
  for(;pos<MENU_PROG_SIZE;++pos) writeByte(0);
}

// generate GROM wrapper
static void genMenuGROM()
{
  const uint8_t *p = grom;
  writeSize(0x81);  // activate GROM 3
  for (uint16_t i = sizeof(grom); i > 0; --i, ++p)
    writeByte(pgm_read_byte(p));
}


/*
 * wait for image selection by user
 */

uint8_t selectImage(uint8_t listen)
{
  receiveBegin();
  receiveBytes(selection, BYTES_TO_SEND, listen);
  receiveEnd();

  // check what has been selected
  switch (selection[9] & 0x0f) {
  case 0x1:  return SEL_FOLDER;
  case 0x2:  return SEL_HELP;
  case 0x3:  return SEL_DUMP;
  case 0x5:  return SEL_GIGA;
  case 0x6:  return SEL_COMPILATION;
  case 0x7:  return 7;
  default:   return SEL_PROGRAM;
  }
}


/*
 * change path after folder selection
 */

uint8_t changeDir()
{
  uint8_t *q = path + path_len;  // move q to \0

  if (selection[0] == '.' && selection[1] == '.') {
    // go to parent directory
    do {
      --q;
      --path_len;
    } while (path_len > 0 && *q != '/');
    *q = 0;  // remove last "/..."
  } else {
    // descend into folder
    if (path_len > 0) {
      *q++ = '/';
      ++path_len;
    }
    const uint8_t *p = selection;
    for (uint8_t i = 8; *p && i > 0; --i) {
      *q++ = *p++;
      ++path_len;
    }
    *q = 0;
  }

  return 1;  // regenerate menu program
}


/*
 * prepare loading of only image on SD card
 */

static void saveFile()
{
  // copy filename base to selection
  uint8_t *p = (uint8_t *)sd_fno.fname;
  uint8_t *q = selection;
  while (*p && *p != '.')
    *q++ = *p++;
  *q = 0;
}


/*
 * load selected image from SD card
 */

__attribute__((noinline)) void loadImage()
{
  uint8_t banks;
  uint32_t size, grom_size = 0, rom_size = 0;

  // begin transmission
  led(1);
  writeBegin();
  setConfig(4);  // Bank-addressed offline loading; CPU remains offline.

  // build path
  uint8_t *p = buildPath("BIN");  // p at '.'
  uint8_t *q = p - 1;  // q at last name char == bank indicator

  // get config
  uint8_t config = selection[9];
  if(config & MODE_GRAM) flash_error(4);  // GRAM is not implemented in this experiment.
  rom_dump_bank = grom_dump_bank = 0;  // no dumps per default

  // load GROM image file
  grom_mask=0x80;
  writeSize(0x80);  // clear and open GROMs
  if (*q == 'G') {  // could also use selection[8]
    size = loadFile(path,123);
    if (size)
      grom_dump_bank = config & MODE_GRAM ? *q : 0;  // GRAM bank to dump
    banks = (size + 8191) / 8192;  // round up
    grom_mask = 0x80 + (1 << banks) - 1;
    writeSize(grom_mask);  // set active GROMs (unary)
    *q = 'C';  // load matching ROM banks
    grom_size = size;
  }

  // load ROM image files
  writeSize(0x00);  // open ROM
  uint8_t cnt = 0;  // at most two banks C and D
  while (cnt < 2) {
    if(cnt && (rom_size & 8191)) {
      if(!pf_open((char *)path)) flash_error(4);
      break;
    }
    size = loadFile(path,rom_size/8192);
    if (size == 0)
      break;
    rom_dump_bank = config & MODE_RAM && cnt == 0 ? *q : 0;  // RAM bank to dump
    rom_size += size;
    if ((*q)++ - cnt++ != 'C')
      break;  // no multi-file
  }

  if ((config & MODE_RAM) && rom_size>8192) flash_error(4);  // single-bank RAM experiment

  // set ROM bank mask
  banks = (rom_size + 8191) / 8192;  // round up
  if (banks > 0) {
    // set mask of available banks
    uint8_t i = 0;
    banks += banks - 1;  // round up again
    while ((banks >>= 1)) ++i; // find left-most bit
    rom_mask = (1 << i) - 1;
    setConfig(0);
    writeSize(rom_mask);  // set ROM bank mask (binary)
  } else {
    // write bogus bytes for end-of-load detection instead
    writeByte(0x99);
    writeByte(0x99);
    setConfig(0);
    writeSize(0);
  }

  image_size = grom_size + rom_size;

  if (image_size == 0) {
    // something went wrong, no file loaded
    flash_error(4);
  }

  // stop transmission
  setConfig(config >> 4);
  writeEnd();
  led(0);
}


// load file and return number of banks occupied
static uint32_t loadFile(uint8_t *fn,uint8_t bank)
{
  UINT bytes_read;
  uint32_t size = 0;

  if (pf_open((const char *)fn))
    return 0;

  uint8_t limit=bank==123?128:((grom_mask&31)?120:128);
  if(sd_fs.fsize>(uint32_t)(limit-bank)*8192) flash_error(4);
  while (bank<128) {
    writeSize(bank++);
    if (pf_read(0, BANK_SIZE, &bytes_read))
      flash_error(3);
    size += bytes_read;
    if (bytes_read < BANK_SIZE)
      break;
  }

  return size;
}


// build path to selected file with given extension
static uint8_t *buildPath(char *ext)
{
  uint8_t *p = selection;
  uint8_t *q = path + path_len;

  // add base filename
  if (path_len > 0)
    *q++ = '/';
  for (uint8_t i = 8; *p && i > 0; --i)
    *q++ = *p++;

  // add extension
  uint8_t *r = q;
  *q++ = '.';
  p = (uint8_t *)ext;
  while (*p)
    *q++ = *p++;
  *q = 0;

  return r;  // returns position of '.'
}


// load help text and branch to help viewer
uint8_t loadHelp()
{
  // take cart offline
  led(1);
  writeBegin();

  UINT bytes_read = 0;
  uint8_t mode = 2;  // 0 = text, 1 = CRLF, 2 = color
  uint8_t color = 0x17;  // default color
  uint8_t line = 0;  // chars per line written
  uint16_t total_lines = 0;  // total lines opened
  uint16_t idx = 0;  // current buffer index

  // open help text
  buildPath("TXT");
  if (pf_open((const char *)path)) {
    led(0);
    return 0;  // no help text -> keep current menu image
  }

  // reset GROM and ROM
  writeSize(0x80);  // kill GROMs
  writeSize(0x00);  // open RAM

  // generate help viewer
  const uint8_t *r = help;
  for (uint16_t i = sizeof(help); i > 0; --i, ++r)
    writeByte(pgm_read_byte(r));

  const uint16_t max_lines = (0x2000 - sizeof(help) - 44) / 40;
                             // currently about 148 lines or 6,44 pages

  // read help text
  while (1) {
    if  (idx == bytes_read) {
      // read next chunk
      if (pf_read(buffer, sizeof(buffer), &bytes_read))
        flash_error(3);
      if (bytes_read == 0)
        break;
      idx = 0;
    }

    // process text: DIS/VAR 38 -> DIS/FIX 40
    uint8_t b = buffer[idx++];
    if (b == 10 || b == 13)  // newline?
      mode = 1;

    if (mode == 2) {  // read color information
      if (idx == 1 && b != '>')
	mode = 0;  // use default color
      else if ('0' <= b && b <= '9')
	color = (color << 4) + (b - '0');
      else if ('A' <= b && b <= 'F')
	color = (color << 4) + (b - 'A' + 10);
      else if ('a' <= b && b <= 'f')
	color = (color << 4) + (b - 'a' + 10);
    }

    if (mode == 1) {  // skip CRLF
      if (line) {
	// pad line to 40 chars
	while (line++ < 40)
	  writeByte(' ');
	line = 0;
      }
      if (b != 10 && b != 13)
	mode = 0;
    }

    if (mode == 0) {  // read text line
      if (line == 0) {
	writeByte(' ');
	++line;
	if (++total_lines > max_lines)
	  break;
      }
      writeByte(b);
      if (++line == 39) {
	writeByte(' ');
	line = 0;
      }
    }
  }

  // complete last line (needs up to 44 extra chars)
  for (uint8_t i = line ? line : 0; i < 38; ++i)
    writeByte(' ');
  writeByte(0x7f);  // end of text char
  writeByte(' ');
  writeByte(0);  // end of text marker
  writeByte(0);
  writeByte(0x07);  // text color (with VDP register)
  writeByte(color);

  // finish off
  writeEnd();
  led(0);

  // wait for help viewer to quit
  receiveBegin();
  receiveBytes(buffer, 1, 0);  // just need one byte to signal
  receiveEnd();

  return 1;  // regenerate menu
}


// reads image data back and writes them to SD card
void dumpImage()
{
  // cart still offline
  led(1);

  if (!rom_dump_bank && !grom_dump_bank)  // nothing to dump
    flash_error(2);

  if (grom_dump_bank)
    dumpBank(grom_dump_bank, grom_mask);
  if (rom_dump_bank)
    dumpBank(rom_dump_bank, rom_mask);

  led(0);
}

static void dumpBank(uint8_t bank, uint8_t mask)
{
  uint8_t *p;
  UINT bw;

  // prepare file for dumping
  for (p = path + path_len; *p != '.'; ++p);  // find extension
  *(--p) = bank;  // restore single file
  if (pf_open((const char *)path))
    flash_error(3);

  // open SRAM for read
  uint8_t dumpmask = mask & 0x80 ? mask | 0x40 : mask;  // don't reset GROM status
  writeBegin();
  writeSize(dumpmask);  // reset SRAM address, but keep mask
  writeEnd();

  // start reading from SRAM
  readBegin();

  // read and dump each SRAM byte
  uint16_t idx = 0;
  for (uint32_t i = image_size; i > 0; --i) {
    buffer[idx++] = readByte();
    if (idx == sizeof(buffer) || i == 1) {  // end of buffer or image
      if (pf_write(buffer, idx, &bw))
        flash_error(3);
      if (bw < sizeof(buffer))
        break;
      idx = 0;
    }
  }

  // finalize write
  if (idx)
    pf_write(0, 0, &bw);
  readEnd();
}


// update CPLD by XSVF player
static void jtag()
{
  led(1);

  if (pf_open("UPDATE.PLD"))
    flash_error(2);
  initJTAGPorts();
  uint8_t res = xsvfExecute();

  led(0);
  flash_error(res ? 2 + res : 0);  // no return
}
