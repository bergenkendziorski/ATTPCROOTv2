#ifndef ENLOSS_C
#define ENLOSS_C

//#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/macro/e12014/simulation/eventGenerator/fissionMC.cpp"

#ifndef __CLING__
#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/AtGenerators/AtTPCFissionGeneratorV3.h"
#endif


//function declarations:
void enlos();
void open_table();

//variable declarations:

void enloss()
{
   open_table();

   int num_points = 10;
   int eloss = 0;
   double energy1 = 500;
   double energy2 = 500;
   double charge1 = 20;
   double charge2 = 20;
   double beta = 0.2271732;
   for (int i = 0; i <= num_points; i++) {
      eloss = 5;
      energy1 -= eloss;
   }

   //auto gen = new AtTPCFissionGeneratorV3();
}

void open_table()
{
   vector<vector<double>> content;
   vector<double> row;
   string line, word;
   double value;

   fstream file ("PbinHe.csv", ios::in);
   if(file.is_open())
   {
      while(getline(file,line))
      {
         row.clear();
	 stringstream str(line);

	 while(getline(str, word, ','))
	 {
	    row.push_back(stod(word));
	 }

	 content.push_back(row);
      }
   }
   else
   {
      cout << "file not found" << endl;
   }

}

#endif
