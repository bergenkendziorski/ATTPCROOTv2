#ifndef ENLOSS_C
#define ENLOSS_C

//#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/macro/e12014/simulation/eventGenerator/fissionMC.cpp"

#ifndef __CLING__
#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/AtGenerators/AtTPCFissionGeneratorV3.h"
#endif


//function declarations:
void enlos();
void open_table_old();

//variable declarations:

void enloss()
{
   //auto gen = new AtTPCFissionGeneratorV3();
   double vertex_loc = 400; //for now this is mm from window. When integrating in I'll need to pull this info
   double theta1 = 45, theta2 = -45; //from perspective of lab in degrees from beam axis
   double curr_dist = vertex_loc;
   double eloss = 0;
   double energy1 = 500, energy2 = 500;
   double mom1 = 50, mom2 = 50;
   double charge1 = 42;
   double charge2 = 85-charge1;
   double beta = 0.2271732;
   auto table = new AtTools::AtELossTable();
   table->LoadLiseTable("LISEum.txt", 207.9323, 0, 4);
   bool finish = false;

   while (!finish){
	   eloss = table->GetEnergyLoss(1000, 100); //first input is energy in MeV, second is distance in mm
	   energy1 -= eloss;

	   curr_dist = 1000;
	   if (curr_dist == 1000) {finish = true;}
   }
   finish = false;



   while (!finish){
	   finish = true;

   }

   



}

void open_table_old()
{
   //Old and unfinished. Delete before pushing this code anywhere
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
