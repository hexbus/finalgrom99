# Cinema example: credits and image identity

Cinema is a cartoon compilation created for this project. The FinalGROM version
is named `CINFRAME.GCS`. Its prepared image is supplied separately as a Beta 1
testing download; it is not stored in the Git source tree.

The image is 134,217,728 bytes (128 MiB). Its SHA-256 is:

```text
33c4a88a263d56cbe024da1cb1731ec1656aab87b7c4aaaa18391ac14103c977
```

This revision includes cooperative bank requests and a sound guard that silences
unused sound channels once per frame. The converter attaches the matching
companions without changing the image. The included recipe rebuilds those
companions; it does not recreate the movie image from the original films.

## Films

The compilation uses these copies, with the original title cards retained:

- [Falling Hare](https://archive.org/details/FallenHeir)
- [A Corny Concerto](https://archive.org/details/ACornyConcerto1943BugsBunny)
- [A Tale of Two Kitties](https://archive.org/details/ATaleOfTwoKitties1942)
- [The Wabbit Who Came to Supper](https://archive.org/details/CameToSupper)

**Published-reported:** the public-domain selection follows the
[ECAT introduction](https://www.youtube.com/watch?v=0AD0Ujng8iA&t=61s) and the
archive catalogues recorded during preparation. Three selected archive items
carried public-domain or CC0 labels; the selected Two Kitties item supplied no
license field. Copyright renewals and separate soundtrack rights were not
independently audited.

## Player and conversion tools

Mike Brent (Tursi) created the conversion tools, timed player and GigaCart
hardware design. See [tividconvert](https://github.com/tursilion/tividconvert)
and the [original player source](https://github.com/tursilion/gigacart/blob/1383b39c5ea9e2b0d916ff4afc718997a20e70ae/code/samples/videoblasterCart.a99).
The project adapted that player for Cinema and then for FinalGROM's cooperative
bank loading.

The [upstream player terms](https://github.com/tursilion/gigacart/blob/1383b39c5ea9e2b0d916ff4afc718997a20e70ae/license.txt)
permit personal use and require an exception for redistribution.
**User-reported permission:** on September 10, 2026, Jon Guidry confirmed that
Tursi granted permission to share the compiled player in this demo for testing.
That permission is specific to this demo; it does not make the player public
domain or change the upstream project's general terms.

Keep these credits with the Cinema testing package. Other cartridge images
remain user-supplied and have their own terms.
