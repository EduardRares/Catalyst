#include "common.hpp"
using namespace fab;
struct Option{Bits one{},two{};int a=-1,b=-1;};
int main(int argc,char**argv){
 if(argc<3)return 1;std::ifstream in(argv[1]);Problem p(in);std::string out=argv[2];
 if(p.nt>std::max(12,6*p.r-3))return 0;
 std::array<std::vector<Option>,3> opts;
 for(int axis=0;axis<3;axis++){
  std::vector<int> poses;
  for(int q=0;q<p.P;q++)if(pop(p.rays[axis][q],p.w))poses.push_back(q);
  // Reverse tracing gives the matching entry of the opposite emitter.
  auto opposite=[&](int q){auto[y,x]=p.rail[q];y+=DR[axis];x+=DC[axis];int last=q;while(y>=0&&y<p.rows&&x>=0&&x<p.cols){if(p.railId[y][x]>=0)last=p.railId[y][x];y+=DR[axis];x+=DC[axis];}return last;};
  opts[axis].push_back({});
  for(int a:poses){Option o;o.a=a;o.one=p.rays[axis][a];opts[axis].push_back(o);}
  for(int i=0;i<(int)poses.size();i++)for(int j=i;j<(int)poses.size();j++){
   Option o;o.a=poses[i];o.b=poses[j];
   for(int w=0;w<p.w;w++){uint64_t a=p.rays[axis][o.a][w],b=p.rays[axis][o.b][w];o.one[w]=a|b;o.two[w]=a&b;}opts[axis].push_back(o);
  }
  // Store exit indices independently for later position assignments.
 }
 Program best=readProgram(out);int matches=0;
 for(const auto&a:opts[0])for(const auto&b:opts[1])for(const auto&c:opts[2]){
  bool match=true;
  for(int w=0;w<p.w;w++){
   uint64_t one=0,two=0,four=0;
   for(const Option*o:{&a,&b,&c}){
    uint64_t v=o->one[w],v2=o->two[w]; // add a 0..2 value
    uint64_t low=v&~v2,carry=one&low;one^=low;
    uint64_t high=v2^carry,carry2=v2&carry;
    uint64_t co=two&high;two^=high;four|=co|carry2;
   }
   uint64_t create=~one&two&~four;
   if((create&p.target[w])!=p.target[w]||(create&p.skirt[w])){match=false;break;}
  }
  if(!match)continue;matches++;
  const Option*o[3]={&a,&b,&c};
  for(int assignments=0;assignments<8;assignments++){
   State dest=p.initial();dest.on=0;
   for(int d=0;d<3;d++){
    int first=o[d]->a,second=o[d]->b;if(assignments>>d&1)std::swap(first,second);
    auto exit=[&](int q){auto[y,x]=p.rail[q];int last=q;y+=DR[d];x+=DC[d];while(y>=0&&y<p.rows&&x>=0&&x<p.cols){if(p.railId[y][x]>=0)last=p.railId[y][x];y+=DR[d];x+=DC[d];}return last;};
    if(first>=0){dest.p[d]=first;dest.on|=1<<d;}
    if(second>=0){dest.p[d+3]=exit(second);dest.on|=1<<(d+3);}
   }
   State s=p.initial();Program candidate;
   bool done=false;while(!done){done=true;Action line=0;for(int d=0;d<6;d++){int z=s.p[d];int v=p.toward(z,dest.p[d]);s.p[d]=z;line|=v<<(2*d);done&=v==0;}if(!done)candidate.push_back(line);}
   Action toggle=0;for(int d=0;d<6;d++)if(dest.on>>d&1)toggle|=3<<(2*d);
   candidate.push_back(toggle);for(int k=1;k<p.need;k++)candidate.push_back(0);
   if((best.empty()||candidate.size()<best.size())&&p.valid(candidate)){best=candidate;writeProgram(out,best);}
  }
 }
 std::cerr<<"static "<<p.nt<<" cells matches "<<matches<<" best "<<best.size()<<"\n";
}
