#include <iostream>
#include "timer.h"

namespace timer_group
{

	static const int SEC_CONVERTER = 1000;//os valores passados no ficheiro de configuracao
	                                      //estao em ms

	timer::timer(){
		//begTime = (saratoga::offset_t)clock();
		begTime = chrono::steady_clock::now();
		elapsedTime = 0;
		name = "";
	}
	timer::timer(saratoga::offset_t elapsed){
		//begTime = (saratoga::offset_t)clock();
		begTime = chrono::steady_clock::now();
		elapsedTime = elapsed/SEC_CONVERTER;
		name = "";
	}
	timer::timer(string str, saratoga::offset_t elapsed){
		begTime = chrono::steady_clock::now();
		elapsedTime = elapsed/SEC_CONVERTER;
		name = str;
	}

	bool timer::elapsed(){//Falta fazer *********************************************
		return false;
		//return (((saratoga::offset_t) clock() - begTime) / CLOCKS_PER_SEC) >= elapsedTime;
	}
	void timer::reset(){ begTime = chrono::steady_clock::now(); }

	bool timer::timedout(){
		chrono::steady_clock::time_point actual = chrono::steady_clock::now();

		chrono::duration<long> time_span = chrono::duration_cast<chrono::duration<long>>(actual - begTime);

		long i = time_span.count();

		if( i>= elapsedTime)
			return true;
		return false;

	}

};

/* TESTE
int main(int argc, char **argv){
	timer_group::timer c(30000);
	int num=0;
	while(1){
		if( c.timedout() ){
			++num;
			c.reset();
			cout<<" passou " <<num <<endl;
		}
	}
	return 0;
}*/
