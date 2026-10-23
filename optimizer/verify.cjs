const fs=require('fs'),vm=require('vm'),assert=require('assert');
const html=fs.readFileSync(process.env.FAB_SANDBOX||require('path').join(require('os').homedir(),'Downloads/level-6/sandbox-level-6.html'),'utf8');
const text=html.slice(html.indexOf('const INSTR_NOOP'),html.indexOf('const globalState =')).replace('    init(gameState, elements);','    return {parseHexInput,parseHexSolution,resetEmitterIndices,applyEmitterStep};');
const ctx={};vm.runInNewContext(text+'\nglobalThis.factory=nanofabricator;',ctx);
function verify(inp,out){
 const tokens=fs.readFileSync(inp,'utf8').trim().split(/\s+/);let at=0,n=+tokens[at++];
 const programs=fs.readFileSync(out,'utf8').trim().split(/\n\s*\n/);assert.equal(programs.length,n);
 let ticks=0,accepted=0;
 for(let i=0;i<n;i++){
  const r=+tokens[at++],rows=+tokens[at++],cols=+tokens[at++],need=+tokens[at++];
  const board=tokens.slice(at,at+rows);at+=rows;
  const gs={radius:0,pattern:{},emitter:{},program:[],acceptedPatterns:0};
  const sim=ctx.factory({gameState:gs,elements:{}});
  sim.parseHexInput(gs,`${r} ${rows} ${cols} ${need}\n${board.join('\n')}`);
  sim.parseHexSolution(gs,programs[i]);
  assert.equal(gs.inputParseErrors.length,0,gs.inputParseErrors.join(';'));
  assert.equal(gs.solutionParseErrors.length,0,gs.solutionParseErrors.join(';'));
  assert(programs[i].split('\n').every(l=>l.trim().split(/\s+/).length===6));
  sim.resetEmitterIndices();for(let k=0;k<gs.program.length;k++)sim.applyEmitterStep(k);
  assert.equal(gs.acceptedPatterns,need,`${inp} case${i+1}`);
  // Remote matter is explicitly permitted by the contest rules.
  ticks+=gs.program.length;accepted+=need;
 }
 assert.equal(at,tokens.length);return {input:inp,patterns:n,ticks,accepted,verified:true};
}
if(process.argv[2]==='--json-lines'){
 const rl=require('readline').createInterface({input:process.stdin});rl.on('line',l=>{try{const q=JSON.parse(l);console.log(JSON.stringify({ok:true,...verify(q.input,q.output)}));}catch(e){console.log(JSON.stringify({ok:false,error:String(e)}));}});
}else for(let i=2;i<process.argv.length;i+=2)console.log(JSON.stringify(verify(process.argv[i],process.argv[i+1])));
