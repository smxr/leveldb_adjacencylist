#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <math.h>
#include <string>
#include <queue>
#include <iostream>
#include <utility>

using namespace std;

inline pair<uint, uint> InverseCantorPairing1(size_t z){
  size_t w = floor((sqrt(8.0 * z + 1) - 1)/2);
  size_t t = (w*w + w) / 2;
  uint y = (uint)(z - t);
  uint x = (uint)(w - y);
  return pair<uint,uint>(x,y);
}


class Point{
public:
	double x = 0;
	double y = 0;
	Point(double xx, double yy){
		x = xx;
		y = yy;
	}
	Point(){}
	Point(Point *p){
		x = p->x;
		y = p->y;
	}
	~Point(){}
	bool equals(Point &p){
		return p.x==x&&p.y==y;
	}
	void print(){
		fprintf(stderr,"POINT (%f %f)\n",x,y);
	}

};

class box{
public:
	double low[2] = {100000.0,100000.0};
	double high[2] = {-100000.0,-100000.0};

	box(){}
	box(double lowx, double lowy, double highx, double highy){
		low[0] = lowx;
		low[1] = lowy;
		high[0] = highx;
		high[1] = highy;
	}
	box(box *b){
		low[0] = b->low[0];
		high[0] = b->high[0];
		low[1] = b->low[1];
		high[1] = b->high[1];
	}

	void update(Point p){
		if(low[0]>p.x){
			low[0] = p.x;
		}
		if(high[0]<p.x){
			high[0] = p.x;
		}

		if(low[1]>p.y){
			low[1] = p.y;
		}
		if(high[1]<p.y){
			high[1] = p.y;
		}
	}

	void update(box &b){
		update(Point(b.low[0],b.low[1]));
		update(Point(b.low[0],b.high[1]));
		update(Point(b.high[0],b.low[1]));
		update(Point(b.high[0],b.high[1]));
	}

	bool intersect(box &target){
		return !(target.low[0]>high[0]||
				 target.high[0]<low[0]||
				 target.low[1]>high[1]||
				 target.high[1]<low[1]);
	}
	bool contain(box &target){
		return target.low[0]>=low[0]&&
			   target.high[0]<=high[0]&&
			   target.low[1]>=low[1]&&
			   target.high[1]<=high[1];
	}
	bool contain(Point &p){
		return p.x>=low[0]&&
			   p.x<=high[0]&&
			   p.y>=low[1]&&
			   p.y<=high[1];
	}

	void print_vertices(){
		fprintf(stderr,"%f %f, %f %f, %f %f, %f %f, %f %f",
					low[0],low[1],
					high[0],low[1],
					high[0],high[1],
					low[0],high[1],
					low[0],low[1]);
	}

	void print(){
		fprintf(stderr,"POLYGON((");
		print_vertices();
		fprintf(stderr,"))\n");
	}
};



