"""Learn optional GPR successors from training traces; score separate traces.

Prediction never replaces a reviewed bank-switch adapter or execution lease.
"""
import argparse,collections,csv,hashlib,importlib.util,json,statistics
from pathlib import Path

def transitions(path,event,banks):
    previous=None
    with path.open(newline='') as f:
        for row in csv.DictReader(f):
            if row.get('event')!=event:continue
            bank=int(row['bank']);stamp=float(row['seconds'])
            if not 0<=bank<banks:raise ValueError('Trace bank outside supplied image')
            if previous:
                if stamp<previous[1]:raise ValueError('Nonmonotonic trace time')
                if bank!=previous[0]:yield previous[0],bank,stamp-previous[1]
            previous=(bank,stamp)

def learn(traces,event,banks,top):
    counts=collections.defaultdict(collections.Counter);times=collections.defaultdict(list)
    for path in traces:
        for a,b,t in transitions(path,event,banks):counts[a][b]+=1;times[a,b].append(t)
    ranked={a:[b for b,_ in sorted(c.items(),key=lambda kv:(-kv[1],statistics.median(times[a,kv[0]]),kv[0]))[:top]] for a,c in counts.items()}
    intervals=[]
    for bank,successors in sorted(ranked.items()):
        if intervals and intervals[-1]['last']==bank-1 and intervals[-1]['successors']==successors:intervals[-1]['last']=bank
        else:intervals.append(dict(first=bank,last=bank,successors=successors))
    return intervals,ranked,counts,times

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);p.add_argument('--train',type=Path,nargs='+',required=True);p.add_argument('--validate',type=Path,nargs='+',required=True);p.add_argument('--event',choices=['select','request'],default='select');p.add_argument('--top',type=int,default=8);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if not 1<=a.top<=8:p.error('Use 1..8 successor starts')
    if {x.resolve() for x in a.train}&{x.resolve() for x in a.validate}:p.error('Training and validation files must be separate')
    raw=a.image.read_bytes()
    if not raw or len(raw)%8192 or len(raw)>134217728:p.error('Supply complete 8 KiB banks, at most 128 MiB')
    intervals,ranked,counts,times=learn(a.train,a.event,len(raw)//8192,a.top)
    model=dict(image_sha256=hashlib.sha256(raw).hexdigest(),intervals=intervals)
    s=importlib.util.spec_from_file_location('gpr',Path(__file__).with_name('build-giga-predict.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);blob=m.compile_model(model)
    tests=list(t for path in a.validate for t in transitions(path,a.event,len(raw)//8192));hits=sum(b in ranked.get(x,[]) for x,b,_ in tests)
    report=dict(training={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in a.train},validation={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in a.validate},validation_transitions=len(tests),successor_matches=hits,successor_match_fraction=hits/len(tests) if tests else None,gpr_bytes=len(blob),
        edges=[dict(source=x,target=b,count=n,median_ms=1000*statistics.median(times[x,b]),minimum_ms=1000*min(times[x,b])) for x,c in sorted(counts.items()) for b,n in sorted(c.items())],
        limitation='Successor coverage only, not residency, SD latency, all-path safety or a statistical guarantee. Distinct files can still contain the same route; choose genuinely held-out sessions.')
    a.output.mkdir(parents=True,exist_ok=False);(a.output/'model.json').write_text(json.dumps(model,indent=2)+'\n');(a.output/'MODEL.GPR').write_bytes(blob);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['validation_transitions','successor_matches','successor_match_fraction','gpr_bytes']},indent=2))
if __name__=='__main__':main()
