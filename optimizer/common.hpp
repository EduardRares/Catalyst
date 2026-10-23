#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace fab {
constexpr int MW=6;
using Bits=std::array<uint64_t,MW>;
using Action=uint16_t;
using Program=std::vector<Action>;
constexpr int DR[6]={0,1,1,0,-1,-1};
constexpr int DC[6]={2,1,-1,-2,-1,1};
inline int pc(uint64_t x){return __builtin_popcountll(x);}
inline double now(){using namespace std::chrono;return duration<double>(steady_clock::now().time_since_epoch()).count();}
inline int pop(const Bits&a,int w){int n=0;for(int i=0;i<w;i++)n+=pc(a[i]);return n;}
inline uint64_t mix(uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
struct State{std::array<uint8_t,6> p{};uint8_t on=0;Bits m{};int made=0;};
struct Problem{
 int r=0,rows=0,cols=0,need=0,P=0,n=0,nt=0,w=0;
 std::vector<std::string> grid;std::vector<std::pair<int,int>> cells,rail;
 std::vector<std::vector<int>> id,railId;
 std::array<std::vector<Bits>,6> rays;
 Bits target{},skirt{};
 explicit Problem(std::istream&in){
  int cases;if(!(in>>cases)||cases!=1)throw std::runtime_error("Expected one case");
  if(!(in>>r>>rows>>cols>>need)||r<1)throw std::runtime_error("Invalid case");
  grid.resize(rows);for(auto&s:grid)in>>s;
  id.assign(rows,std::vector<int>(cols,-1));railId=id;
  for(int y=0;y<rows;y++)for(int x=0;x<cols;x++)if(grid[y][x]=='M')cells.push_back({y,x});
  nt=cells.size();
  std::vector<std::vector<bool>> seen(rows,std::vector<bool>(cols));
  for(auto[y,x]:cells)seen[y][x]=true;
  auto targets=cells;
  for(auto[y,x]:targets)for(int d=0;d<6;d++){
   int yy=y+DR[d],xx=x+DC[d];
   if(yy>=0&&yy<rows&&xx>=0&&xx<cols&&grid[yy][xx]=='O'&&!seen[yy][xx]){seen[yy][xx]=true;cells.push_back({yy,xx});}
  }
  n=cells.size();w=(n+63)/64;if(w>MW||!nt)throw std::runtime_error("Unsupported pattern size");
  for(int i=0;i<n;i++){auto[y,x]=cells[i];id[y][x]=i;(i<nt?target:skirt)[i/64]|=1ULL<<(i%64);}
  int dy[6]={-1,0,1,1,0,-1},dx[6]={1,2,1,-1,-2,-1};
  int y=r,x=0;for(int d=0;d<6;d++)for(int j=0;j<r;j++){railId[y][x]=rail.size();rail.push_back({y,x});y+=dy[d];x+=dx[d];}P=rail.size();
  for(int d=0;d<6;d++){
   rays[d].resize(P);
   for(int p=0;p<P;p++){auto[yy,xx]=rail[p];yy+=DR[d];xx+=DC[d];
    while(yy>=0&&yy<rows&&xx>=0&&xx<cols){int k=id[yy][xx];if(k>=0)rays[d][p][k/64]|=1ULL<<(k%64);yy+=DR[d];xx+=DC[d];}
   }
  }
 }
 State initial()const{State s;for(int d=0;d<6;d++)s.p[d]=d*r;return s;}
 uint64_t config(const State&s)const{uint64_t h=s.on;for(int d=0;d<6;d++)h=(h<<7)|s.p[d];return h;}
 uint64_t hash(const State&s)const{uint64_t h=mix(config(s)^uint64_t(s.made)*0x123456789abcdefULL);for(int j=0;j<w;j++)h=mix(h^s.m[j]);return h;}
 bool equal(const State&a,const State&b)const{return a.on==b.on&&a.p==b.p&&a.made==b.made&&a.m==b.m;}
 void effects(const State&s,Bits&create,Bits&destroy)const{
  for(int j=0;j<w;j++){
   uint64_t one=0,two=0,four=0;
   for(int d=0;d<6;d++)if(s.on>>d&1){uint64_t b=rays[d][s.p[d]][j],c=one&b;one^=b;uint64_t c2=two&c;two^=c;four|=c2;}
   create[j]=~one&two&~four;destroy[j]=four|(one&two);
  }
 }
 bool apply(State&s,Action a)const{
  for(int d=0;d<6;d++){int v=a>>(2*d)&3;if(v==1)s.p[d]=(s.p[d]+1)%P;else if(v==2)s.p[d]=(s.p[d]+P-1)%P;else if(v==3)s.on^=1<<d;}
  Bits c{},z{};effects(s,c,z);bool match=true;
  for(int j=0;j<w;j++){s.m[j]=(s.m[j]|c[j])&~z[j]&(target[j]|skirt[j]);if((s.m[j]&target[j])!=target[j]||(s.m[j]&skirt[j]))match=false;}
  if(match){s.m={};s.made++;}return match;
 }
 bool valid(const Program&prog)const{State s=initial();for(auto a:prog){apply(s,a);if(s.made>need)return false;}return s.made==need;}
 int distance(int a,int b)const{int d=std::abs(a-b);return std::min(d,P-d);}
 int toward(int& a,int b)const{if(a==b)return 0;int f=(b-a+P)%P;if(f<=P-f){a=(a+1)%P;return 1;}a=(a+P-1)%P;return 2;}
};
inline Program readProgram(const std::string&path){std::ifstream f(path);Program p;char c;while(f>>c){Action a=0;for(int d=0;d<6;d++){if(d&&!(f>>c))throw std::runtime_error("Incomplete instruction");int v=c=='-'?0:c=='F'?1:c=='B'?2:c=='T'?3:-1;if(v<0)throw std::runtime_error("Bad instruction");a|=v<<(2*d);}p.push_back(a);}return p;}
inline void writeProgram(const std::string&path,const Program&p){std::string tmp=path+".tmp";{std::ofstream f(tmp);const char*v="-FBT";for(auto a:p){for(int d=0;d<6;d++){if(d)f<<' ';f<<v[a>>(2*d)&3];}f<<'\n';}}std::rename(tmp.c_str(),path.c_str());}
inline Program trim(const Problem&p,const Program&prog){State s=p.initial();Program q;for(auto a:prog){q.push_back(a);p.apply(s,a);if(s.made==p.need)break;}return q;}
inline bool saveBetter(const Problem&p,const Program&candidate,const std::string&out){Program q=trim(p,candidate),old=readProgram(out);if(!p.valid(q))return false;if(!old.empty()&&q.size()>=old.size())return false;writeProgram(out,q);return true;}
}
