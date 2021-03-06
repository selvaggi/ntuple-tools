//
//  Helpers.hpp
//
//  Created by Jeremi Niedziela on 13/06/2018.
//

#ifndef GeneticHelpers_h
#define GeneticHelpers_h

#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <bitset>
#include <memory>
#include <chrono>

#include <TMath.h>

enum EParam{
  kCriticalDistanceEE,
  kCriticalDistanceFH,
  kCriticalDistanceBH,
  kAssignmentDistanceEE,
  kAssignmentDistanceFH,
  kAssignmentDistanceBH,
  kDeltacEE,
  kDeltacFH,
  kDeltacBH,
  kKappa,
  kEnergyThreshold,
  kMatchingDistance,
  kKernel,
  kNparams
};

const double paramMin[kNparams] = {
  0.0,  ///< min critical distance EE
  2.0,  ///< min critical distance FH (all dies below 2.0)
  5.0,  ///< min critical distance BH (all dies below 5.0)
  0.0,  ///< min assignment distance EE
  2.0,  ///< min assignment distance FH
  5.0,  ///< min assignment distance BH
  0.01, ///< min critical delta EE (too small - algorithm cannot find clusters)
  0.01, ///< min critical delta FH
  0.01, ///< min critical delta BH
  1.0,  ///< min kappa  // below 1.0 algorithm doesn't work
  2.0,  ///< min energy threshold
  0.0,  ///< min matching distance
 -0.49  ///< min kernel index
};

const double paramMax[kNparams] = {
  20.0, ///< max critical distance EE (almost all dies above 20)
  30.0, ///< max critical distance FH
  50.0, ///< max critical distance BH
  20.0, ///< max assignment distance EE
  30.0, ///< max assignment distance FH
  50.0, ///< max assignment distance BH
  30.0, ///< max critical delta EE (above ~30 problems start to occur)
  30.0, ///< max critical delta FH
  40.0, ///< max critical delta BH
  150.0,///< max kappa (above 150 almost all creatures fail completely)
  10.0, ///< max energy threshold
  30.0, ///< max matching distance
  2.49  ///< max kernel index
};

const double paramStart[kNparams] = {
  2.00, ///< initial critical distance EE
  2.00, ///< initial critical distance FH
  2.00, ///< initial critical distance BH
  2.00, ///< initial assignemnt distance EE
  2.00, ///< initial assignemnt distance FH
  2.00, ///< initial assignemnt distance BH
  2.00, ///< initial critical delta EE
  2.00, ///< initial critical delta FH
  5.00, ///< initial critical delta BH
  9.00, ///< initial kappa
  3.00, ///< initial energy threshold
  5.00, ///< initial matching distance
  0.0   ///< initial kernel index (0 - step, 1 - gaus, 2 - exp)
};
  
static const char* paramTitle[kNparams] = {
  "critDistEE",
  "critDistFH",
  "critDistBH",
  "assignDistEE",
  "assignDistFH",
  "assignDistBH",
  "deltaEE",
  "deltaFH",
  "deltaBH",
  "kappa",
  "eMin",
  "matchingDist",
  "kernel"
};

enum ECrossover{
  kUniform,           ///< each bit has a chance to be exchaned between parents
  kSinglePoint,       ///< one chromosome gets crossed, the rest stays the same or is exchaned intact
  kFixedSinglePoint,  ///< one chromosome gets crossed at point that doesn't modify any of the parameters, the rest stays the same or is exchaned intact
  kMultiPoint,        ///< each chromosome is crossed at a random point
  kNcrossover
};

static std::string crossoverName[kNcrossover] = {
  "Uniform",
  "Single point",
  "Fixed single point",
  "Multi point"
};

inline double RandDouble(double min, double max)
{
  return min + static_cast<double>(rand()) /( static_cast<double>(RAND_MAX/(max-min)));
}

inline float RandFloat(float min, float max)
{
  return min + static_cast<float>(rand()) /( static_cast<float>(RAND_MAX/(max-min)));
}

inline int RandInt(int min, int max)
{
  return min + (rand() % static_cast<int>(max - min + 1));
}

inline bool RandBool()
{
  return rand() % 2;
}

/// Inverts value of bit at position pos in bits
inline void ReverseBit(uint64_t &bits, int pos)
{
  uint64_t mask = pow(2,pos);
  if((bits&mask)==0) bits = bits|mask;
  else bits = bits&(~mask);
}

inline void PrintBits(uint64_t bits)
{
  std::bitset<64> x(bits);
  std::cout<<x<<std::endl;
}

template<class T>
T BitSize(T&)
{
  return std::numeric_limits<T>::digits;
}

template<class T>
inline void UpdateParamValue(std::string configPath, std::string keyToReplace, T newValue){
  std::string tmpName = "tmp/tmp_"+std::to_string(RandInt(0, 100000))+".md";
  
//  std::cout<<"tmp name:"<<tmpName<<std::endl;
//  std::cout<<"config path:"<<configPath<<std::endl;
  
  std::ifstream is_file(configPath);
  std::ofstream outputFile;
  outputFile.open(tmpName);
  
  std::string line;
  while(getline(is_file, line)){
    std::istringstream is_line(line);
    std::string key;
    
    if( std::getline(is_line, key, ':')){
      std::string value;
      
      if(std::getline(is_line, value)){
        if(key==keyToReplace) outputFile<<key<<":\t"<<newValue<<std::endl;
        else                  outputFile<<line<<std::endl;
      }
      else outputFile<<line<<std::endl;
    }
    else  outputFile<<line<<std::endl;
  }
  outputFile.close();
//  std::cout<<"Executing: "<<("mv "+tmpName+" "+configPath)<<std::endl;
  system(("mv "+tmpName+" "+configPath).c_str());
}

template<class T>
void GetParamFromConfig(std::string configPath, std::string keyToFind, T &returnValue){
  std::ifstream is_file(configPath);
  
  std::string line;
  while(getline(is_file, line)){
    std::istringstream is_line(line);
    std::string key;
    if( std::getline(is_line, key, ':')){
      std::string value;
      if(std::getline(is_line, value)){
        if(key==keyToFind){
          std::istringstream is_value(value);
          returnValue = decltype(returnValue)(is_value);
          return;
        }
      }
    }
  }
  return;
}

/// This struct holds values showing how successful clustering was
struct ClusteringOutput {
  
  /// Default constructor
  ClusteringOutput(){
    resolutionMean      = 99999;
    resolutionSigma     = 99999;
    separationMean      = 99999;
    separationSigma     = 99999;
    containmentMean     = 99999;
    containmentSigma    = 99999;
    deltaNclustersMean  = 99999;
    deltaNclustersSigma = 99999;
    nRecoFailed         = 99999;
    nCantMatchRecSim    = 99999;
    nFakeRec            = 99999;
  }
  
  /// Prints clustering results
  void Print(){
    std::cout<<"Resolution:"<<resolutionMean<<" +/- "<<resolutionSigma<<std::endl;
    std::cout<<"Separation:"<<separationMean<<" +/- "<<separationSigma<<std::endl;
    std::cout<<"Containment:"<<containmentMean<<" +/- "<<containmentSigma<<std::endl;
    std::cout<<"Delta N clusters:"<<deltaNclustersMean<<" +/- "<<deltaNclustersSigma<<std::endl;
    std::cout<<"\% of event-layers where algo failed to find sim clusters:"<<nRecoFailed<<std::endl;
    std::cout<<"\% of event-layers were rec and sim clusters couldn't be matched:"<<nCantMatchRecSim<<std::endl;
    std::cout<<"\% of fake rec clusters (those that don't match any sim cluster):"<<nFakeRec<<std::endl;
  }
  
  double resolutionMean;    ///< Mean resolution defined as (E_rec - E_sim)/E_sim
  double resolutionSigma;   ///< Std dev of the resolution defined as (E_rec - E_sim)/E_sim
  double separationMean;    ///< Mean of the separation factor defined as distance/(sigma_1+sigma_2)
  double separationSigma;   ///< Std dev of the separation factor defined as distance/(sigma_1+sigma_2)
  double containmentMean;   ///< Mean of the containment defined as Ei_matched/Ei_totalSim
  double containmentSigma;  ///< Std dev of the containment defined as Ei_matched/Ei_totalSim
  double deltaNclustersMean;///< Mean of the (N_simHits - N_recHits)/N_simHits
  double deltaNclustersSigma;
  double nRecoFailed;
  double nCantMatchRecSim;
  double nFakeRec;
  
};

/// Read specified file containing ouput of clusterization and load it to the ClusteringOutput object.
/// \param fileName Path to the output file.
/// \return ClusteringOutput object with fields populated from the output file specified.
inline ClusteringOutput ReadOutput(std::string fileName)
{
  std::ifstream is_file(fileName);
  std::string line;
  int iter=0;
  ClusteringOutput output = ClusteringOutput();
  
  while(getline(is_file, line)){
    std::istringstream is_line(line);
    if(iter==0) is_line >> output.resolutionMean;
    if(iter==1) is_line >> output.resolutionSigma;
    if(iter==2) is_line >> output.separationMean;
    if(iter==3) is_line >> output.separationSigma;
    if(iter==4) is_line >> output.containmentMean;
    if(iter==5) is_line >> output.containmentSigma;
    if(iter==6) is_line >> output.deltaNclustersMean;
    if(iter==7) is_line >> output.deltaNclustersSigma;
    if(iter==8) is_line >> output.nRecoFailed;
    if(iter==9) is_line >> output.nCantMatchRecSim;
    if(iter==10)is_line >> output.nFakeRec;
    iter++;
  }
  return output;
}
  
#endif /* Helpers_h */
