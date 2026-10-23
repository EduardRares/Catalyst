#include "common.hpp"
using namespace fab;
struct Half{std::array<uint8_t,3>p;uint8_t on;Bits h1{},h2{},h3{};};
struct Node{State s;float score=0;int parent=-1;Action action=0;};
struct Seed{State s;Program program;float value;};
static uint64_t rnd(uint64_t&x){x^=x<<13;x^=x>>7;x^=x<<17;return x;}
float value(const Problem&p,const State&s,float penalty){int good=0,bad=0;for(int j=0;j<p.w;j++){good+=pc(s.m[j]&p.target[j]);bad+=pc(s.m[j]&p.skirt[j]);}return s.made*p.nt*2+good+0.25f*good*good/p.nt-penalty*bad;}
std::array<Half,64> halves(const Problem&p,const State&s,int base){
 std::array<Half,64>v;
 for(int a=0;a<64;a++){
  auto&h=v[a];h.on=0;
  for(int k=0;k<3;k++){int d=base+k,z=a>>(2*k)&3,pos=s.p[d];bool on=s.on>>d&1;if(z==1)pos=(pos+1)%p.P;else if(z==2)pos=(pos+p.P-1)%p.P;else if(z==3)on=!on;h.p[k]=pos;if(on)h.on|=1<<d;}
  for(int w=0;w<p.w;w++){
   uint64_t b[3];for(int k=0;k<3;k++)b[k]=(h.on>>(base+k)&1)?p.rays[base+k][h.p[k]][w]:0;
   h.h1[w]=b[0]|b[1]|b[2];h.h2[w]=(b[0]&b[1])|(b[0]&b[2])|(b[1]&b[2]);h.h3[w]=b[0]&b[1]&b[2];
  }
 }
 return v;
}
std::vector<Seed> makeSeeds(const Problem&p,uint64_t&random){
 std::vector<Seed>seeds;seeds.push_back({p.initial(),{},0});
 std::vector<std::array<int,6>> entries(p.nt);
 for(int i=0;i<p.nt;i++)for(int d=0;d<6;d++){auto[y,x]=p.cells[i];while(p.railId[y][x]<0){y-=DR[d];x-=DC[d];}entries[i][d]=p.railId[y][x];}
 std::vector<std::pair<float,State>>configs;
 for(int trial=0;trial<6000;trial++){
  State s=p.initial();s.on=0;
  for(int d=0;d<6;d++){s.p[d]=entries[rnd(random)%p.nt][d];if((rnd(random)%8)!=0)s.on|=1<<d;}
  Bits c{},z{};p.effects(s,c,z);s.m=c;int good=0,bad=0,travel=0;
  for(int w=0;w<p.w;w++){good+=pc(c[w]&p.target[w]);bad+=pc(c[w]&p.skirt[w]);s.m[w]&=p.target[w]|p.skirt[w];}
  for(int d=0;d<6;d++)if(s.on>>d&1)travel=std::max(travel,p.distance(d*p.r,s.p[d]));else s.p[d]=d*p.r;
  float score=good-1.0f*bad-0.10f*travel;
  configs.push_back({score,s});
 }
 std::sort(configs.begin(),configs.end(),[](auto&a,auto&b){return a.first>b.first;});
 std::unordered_set<uint64_t>seen;
 for(auto&[score,dest]:configs){
  if(seeds.size()>=13)break;if(!seen.insert(p.config(dest)).second)continue;
  State s=p.initial();Program program;bool done=false;
  while(!done){done=true;Action a=0;for(int d=0;d<6;d++){int pos=s.p[d],v=p.toward(pos,dest.p[d]);s.p[d]=pos;a|=v<<(2*d);done&=v==0;}if(!done)program.push_back(a);}
  s=p.initial();for(auto a:program)p.apply(s,a);Action t=0;for(int d=0;d<6;d++)if(dest.on>>d&1)t|=3<<(2*d);p.apply(s,t);program.push_back(t);
  seeds.push_back({s,program,score});
 }
 return seeds;
}
Program search(const Problem&p,const Seed&seed,int width,int limit,double deadline,float penalty,uint64_t&random){
 int goalMade=seed.s.made+1; if(seed.s.made>=p.need)return seed.program;
 std::vector<Node>arena(1);arena[0].s=seed.s;std::vector<int>front{0};
 float highest=-1e9;int stagnant=0;
 std::unordered_map<uint64_t,int>closed;closed.reserve(width*limit*2);
 for(int depth=0;depth<limit&&(now()<deadline);depth++){
  std::vector<Node>pool;pool.reserve(width*24);
  int found=-1;
  for(int parent:front){
   if(now()>deadline)return {};
   const State s=arena[parent].s;
   auto A=halves(p,s,0),B=halves(p,s,3);
   std::vector<Node>local;local.reserve(4096);
   for(int a=0;a<64;a++)for(int b=0;b<64;b++){
    Node q;q.s=s;q.parent=parent;q.action=a|(b<<6);q.s.on=A[a].on|B[b].on;
    for(int d=0;d<3;d++){q.s.p[d]=A[a].p[d];q.s.p[d+3]=B[b].p[d];}
    bool match=true;int good=0,bad=0;
    for(int w=0;w<p.w;w++){
     uint64_t a1=A[a].h1[w],a2=A[a].h2[w],a3=A[a].h3[w],b1=B[b].h1[w],b2=B[b].h2[w],b3=B[b].h3[w];
     uint64_t destroy=a3|b3|(a2&b1)|(a1&b2);
     uint64_t create=((a2&~a3)&~b1)|((b2&~b3)&~a1)|((a1&~a2)&(b1&~b2));
     q.s.m[w]=(s.m[w]|create)&~destroy&(p.target[w]|p.skirt[w]);
     int g=pc(q.s.m[w]&p.target[w]),z=pc(q.s.m[w]&p.skirt[w]);good+=g;bad+=z;
     if((q.s.m[w]&p.target[w])!=p.target[w]||z)match=false;
    }
    if(match){q.s.made++;q.s.m={};good=0;bad=0;}
    if(q.s.made>=goalMade){arena.push_back(q);found=arena.size()-1;break;}
    float jitter=float(rnd(random)&65535)*0.00000005f;
    q.score=q.s.made*p.nt*2+good+0.25f*good*good/p.nt-penalty*bad+jitter;
    local.push_back(q);
   }
   if(found>=0)break;
   int take=std::min(24,(int)local.size());
   if((int)local.size()>take)std::nth_element(local.begin(),local.begin()+take,local.end(),[](auto&a,auto&b){return a.score>b.score;});
   for(int i=0;i<take;i++)pool.push_back(std::move(local[i]));
  }
  if(found>=0){Program suffix;for(int j=found;arena[j].parent>=0;j=arena[j].parent)suffix.push_back(arena[j].action);std::reverse(suffix.begin(),suffix.end());Program result=seed.program;result.insert(result.end(),suffix.begin(),suffix.end());return result;}
  if(pool.empty())break;
  std::sort(pool.begin(),pool.end(),[](auto&a,auto&b){return a.score>b.score;});
  front.clear();std::unordered_set<uint64_t>unique;
  for(auto&q:pool){
   uint64_t h=p.hash(q.s);if(!unique.insert(h).second)continue;
   auto it=closed.find(h);if(it!=closed.end()&&it->second<=depth)continue;
   closed[h]=depth;front.push_back(arena.size());arena.push_back(q);if((int)front.size()>=width)break;
  }
  if(front.empty())break;
  float current=arena[front[0]].score;if(current>highest+0.05f){highest=current;stagnant=0;}else stagnant++;
  if(stagnant>12)break;
 }
 return {};
}
int main(int argc,char**argv){
 if(argc<3)return 1;std::ifstream in(argv[1]);Problem p(in);std::string out=argv[2];
 double seconds=2;int width=24;uint64_t random=42;
 for(int i=3;i+1<argc;i+=2){std::string a=argv[i];if(a=="--seconds")seconds=std::stod(argv[i+1]);if(a=="--width")width=std::stoi(argv[i+1]);if(a=="--seed")random=std::stoull(argv[i+1]);}
 double end=now()+seconds;auto seeds=makeSeeds(p,random);Program best=readProgram(out);
 int trials=0;
 while(now()<end){
  Seed seed=seeds[trials%seeds.size()];float penalty[3]={0.5f,1.5f,3.0f};
  double slice=std::min(end,now()+std::max(.12,seconds/8));
  int limit=std::min(500,(int)best.size()-(int)seed.program.size()-1);
  auto candidate=search(p,seed,width,limit,slice,penalty[(trials/3)%3],random);
  if(!candidate.empty()){
   State state=p.initial();for(auto a:candidate)p.apply(state,a);
   while(state.made<p.need && now()<end && candidate.size()<best.size()){
    Seed next{state,candidate,0};
    auto extension=search(p,next,width,std::min(100,(int)best.size()-(int)candidate.size()-1),std::min(end,now()+std::max(.12,seconds/8)),penalty[(trials/3)%3],random);
    if(extension.empty())break;
    candidate=extension;state=p.initial();for(auto a:candidate)p.apply(state,a);
   }
  }
  if(!candidate.empty()&&candidate.size()<best.size()&&p.valid(candidate)){best=candidate;writeProgram(out,best);std::cerr<<"improved "<<best.size()<<" trial "<<trials<<"\n";}
  trials++;
 }
 std::cerr<<"beam final "<<best.size()<<" trials "<<trials<<"\n";
}
