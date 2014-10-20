#include "math.h"     // математические функции
#include "PACRXPlc.h" //процедуры для контроллера PAC
//#include "plcc9070.h" //процедуры для контроллера 90-70

//Весовые коэффициенты для первого слоя
static float W1[10][3] ={{-0.00066743122035897099, -0.11206446014488511, 0.18019925126429742},
					{0.0003889544485913661, 0.15069729232506487, -0.081334441477852293}	,
					{0.00026009607440958632, 0.063526490616979098, 0.011797379523859338},
					{-0.00056972120462246755, -0.19132461828198041, 0.061199285317524492},
					{-0.099973635598832711, 2.8634131297756991, 16.717207768677095},
					{-0.00066887787432494861, -0.18010598749241391, 0.050904474640054258},
					{0.0059220906350217883, 0.11468973909442041, -0.0058271304410389968},
					{0.00040072708699105176, 0.15224871553489033, -0.080414211755419351},
					{-0.00029108067581634574, 0.221865316933716, 0.02397429792640738},
					{0.0030605711313751579, -0.096052630538237732, -0.032110165041128243}};
//Весовые коэффициенты для второго слоя
static float W2[] = {0.2014952594481072,-17.665856907001295,0.32895145996666803,-0.16313906729449315,-0.010968372198123189,-0.16608301452374061,0.4906858391457794,18.608278949900093,0.057164287403765564,-0.050891845936561014};
//Смещения для первого слоя
static float off_1[] = {-6.6059587578557615,-4.3526093218402542,-9.6125377715099489,15.34654034935151,-10.41504843932894,
						15.297045460594161,-11.215883022458025,-4.7902903485636283,-4.7770801252432253,-1.8726623455616338};
//Смещение для второго слоя						
#define off_2 0.25321147312764225;	

						
//***************************************************** tansig ******************************************************
//Функция активации гиперболический тангенс
float tansig( float in )
{
float out;
out = (2 / (1+exp(-2*in))) - 1;
return out;
}

//*************************************************** NeuroNet ***************************************************
//Нейронная сеть 3-10-1
//3 входа, 10 нейронов в скрытом (первом) слое, один нейрон в выходном (втором) слое  
float NeuroNet( float Nst, float Ps, float Pd)
{
float IN[3], sum, LW_1[10], LW_2;
int i,j;

IN[0]= Nst;
//Ограничения по оборотам
if (IN[0] <13500) IN[0]=13500;
if (IN[0] >22500) IN[0]=22500;

//Переводим из кгс/см2 в атм
IN[1]= Ps / 0.010197 / 101.325;
//Ограничения по входному давлению
if (IN[1] <30) IN[1]=30;
if (IN[1] >55) IN[1]=55;

//Переводим из кгс/см2 в атм
IN[2]= Pd / 0.010197 / 101.325;
//Ограничения по выходному давлению
if (IN[2] <40) IN[2]=40;
if (IN[2] >160) IN[2]=160;


//Первый слой
for (j= 0; j<10; j++)
	{
	sum= 0;
	for (i= 0; i < 3; i++)
		sum= sum + IN[i] * W1[j][i];

	sum= sum + off_1[j];
	//выход i-го нейрона первого слоя
	LW_1[j]= tansig(sum);
	}  //for j:= 1 to 10 do

sum= 0;
//Второй слой
for (i= 0; i<10; i++) 
	sum= sum + LW_1[i] * W2[i];

//выход нейрона второго слоя	
LW_2= sum + off_2;
return LW_2;
}

//***************************************************** main **********************************************************
void GefMain(/*Входы*/IN, 
			 /*Выходы*/ OUT, 
			 /*Константы*/ Const)
float *IN;
float *OUT;
float *Const;

//void GefMain(/*Входы*/float ON_Calc,N,float *P_IN, float *P_OUT, float *T_IN, 
//				/*Выходы*/ float *Ky, float *Q_35ata,  float *Q_pr,
//				/*Константы*/ float Nnom, float *Q_min, float *p_g, float *B)

/*
#define Nnom 22300 	//Номинальные обороты нагнетателя
#define Q_min 12.3  	//Запас по помпажу
#define p_g 0.68 		// плотность газа кг/м3
#define B 750		// барометрическое давление мм.рт.ст
*/
{
float B_, Pin, Tin, p_, R, Tkr, Tpr, Pkr, Ppr, Zin, p_in, Q, Rc;
float N,P_IN,P_OUT,T_IN, Ky, Q_35ata, Q_pr, Nnom, Q_min, p_g, B, ON_Calc;

ON_Calc= IN[0];
N= IN[1];
P_IN= IN[2]; 	//кгс/см2
P_OUT= IN[3];	//кгс/см2
T_IN= IN[4];	//гр.С
Q_35ata = NeuroNet (N, P_IN, P_OUT);

Nnom= Const[0];
Q_min= Const[1];
p_g= Const[2];
B= Const[3];

//Если расчет включен, то продолжаем
if (ON_Calc == 1)
	{
	//Степень сжатия
	if (P_IN != 0) 
		Rc = P_OUT / P_IN;	
	else
		Rc = 1;
	


	//Барометрическое давление, (ks/cm2)
	B_= B * 1.0332/760.0;

	//Давление на входе в нагнетатель, (ks/cm2)
	Pin= P_IN /* * 0.010197*/ + B_;
	if (Pin == 0.0) 
		Pin= 0.0000001; 

	//Температура на входе в нагнетатель, (грK)
	Tin= T_IN+273.2;

	//Относительная плотность газа
	p_= p_g/1.205;

	//Газовая постоянная
	if (p_!= 0.0) 
		R= 29.27/p_;
	else 
		R= 1;

	//Псевдокритическая температура
	Tkr= 162.8 * (0.613+p_);

	//Псевдокритическое давление
	Pkr= 47.9-p_;
	if (Pkr == 0.0) Pkr= 0.0001; 

	//Предельная температура
	if (Tkr != 0.0) 
		Tpr= Tin/Tkr;
	else 
		Tpr= 1;
	
	if (Tpr == 0.0) 
		Tpr= 0.01; 

	//Предельное давление
	Ppr= Pin/Pkr;
	if (Ppr == 0.0) 
		Ppr= 0.0001; 


	//Коэффициент сжимаемости
	Zin= 1.0 - (((0.41 / (Tpr*Tpr*Tpr)) - (0.061/Tpr)) * Ppr)-(0.04 * (Tpr*Tpr)/(Tpr*Tpr*Tpr));	
		
	//Плотность на входе в нагнетатель	
	if (Zin * Tin * R != 0) 
		p_in = (Pin * 10000) / (Zin * Tin * R);
	else 
		p_in= 1;
	
	if (p_in == 0.0) 
		p_in= 0.0001; 

	//Производительность (м3/час)
	Q = (Q_35ata * p_g) / (0.00144 * p_in);
	
	//Приведенная производительность (м3/час)	
	if (N != 0) 
		Q_pr = 	Q * (Nnom / N);
	else 
		Q_pr= 0;
	
	//Запас по помпажу %
	if (Q_min != 0) 
		Ky = 100 * (Q_pr - Q_min) / Q_min;
	else 
		Ky = 0;

	OUT[0]= Ky;
	OUT[1]= Q_pr;
	OUT[2]= Q_35ata;
	} // if (ON_Calc == 1)
//Если расчет выключен, то возвращаем нулевые значения
else 
	{
	OUT[0]= 0;
	OUT[1]= 0;
	OUT[2]= 0;
	}
}
