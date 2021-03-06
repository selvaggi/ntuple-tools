#include "Event.hpp"
#include "RecHitCalibration.hpp"
#include "ImagingAlgo.hpp"
#include "Helpers.hpp"
#include "ConfigurationManager.hpp"
#include "Helpers.hpp"

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TH2D.h>

#include <string>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <utility>
#include <memory>

using namespace std;

int main(int argc, char* argv[])
{
  if(argc != 2){
    cout<<"Usage: algoBenchmark path_to_config"<<endl;
    exit(0);
  }
  string configPath(argv[1]);
  auto config = ConfigurationManager::Instance(configPath);
  
  gROOT->ProcessLine(".L loader.C+");

  std::system(("mkdir -p "+config->GetOutputPath()).c_str());

  ImagingAlgo *algo = new ImagingAlgo();
  
  for(int nTupleIter=config->GetMinNtuple();nTupleIter<=config->GetMaxNtuple();nTupleIter++){
    cout<<"\nCurrent ntup: "<<nTupleIter<<endl;

    TFile *inFile = TFile::Open(Form("%s%i.root",config->GetInputPath().c_str(),nTupleIter));
    TTree *tree = (TTree*)inFile->Get("ana/hgc");
    if(!tree){
      cout<<"Failed to load MC data. Trying to load testbeam data."<<endl;
      tree = (TTree*)inFile->Get("rechitntupler/hits");
    }
    long long nEvents = tree->GetEntries();


    cout<<"\n\nLoading ntuple...";
    auto start = now();
    unique_ptr<Event> hgCalEvent(new Event(tree));
    auto end = now();
    cout<<" done ("<<duration(start,end)<<" s)"<<endl;

    cout<<"n entries:"<<nEvents<<endl;

    // start event loop
    for(int iEvent=0;iEvent<nEvents;iEvent++){
      if(iEvent>config->GetMaxEventsPerTuple()) break;
      auto startEvent = now();
      hgCalEvent->GoToEvent(iEvent);

      bool isTestBeam = hgCalEvent->IsTestBeam();
      
      // check if particles reached EE
      if(config->GetReachedEEonly() && !isTestBeam){
        bool skipEvent = false;
        for(auto reachedEE : *(hgCalEvent->GetGenParticles()->GetReachedEE())){
          if(reachedEE==0){
            skipEvent = true;
            break;
          }
        }
        if(skipEvent) continue;
      }
      string eventDir = config->GetOutputPath()+"/ntup"+to_string(nTupleIter)+"/event"+to_string(iEvent);
      std::system(("mkdir -p "+eventDir).c_str());

      cout<<"\nCurrent event:"<<iEvent<<endl;

      shared_ptr<RecHits> recHitsRaw = hgCalEvent->GetRecHits();
      shared_ptr<SimClusters> simClusters = hgCalEvent->GetSimClusters();

      // get simulated hits associated with a cluster
      vector<unique_ptr<RecHits>> simHitsPerClusterArray;
      if(!isTestBeam){
        cout<<"preparing simulated hits and clusters...";
        start = now();
        recHitsRaw->GetHitsPerSimCluster(simHitsPerClusterArray, simClusters);
        end = now();
        cout<<" done ("<<duration(start,end)<<" s)"<<endl;
      }

      // get original 2d clusters from the tree
      if(!isTestBeam){
        cout<<"preparing 2d clusters from the original reco...";
        start = now();
        shared_ptr<Clusters2D> clusters2D = hgCalEvent->GetClusters2D();
        end = now();
        cout<<" done ("<<duration(start,end)<<" s)"<<endl;
      }
        
      // re-run clustering with HGCalAlgo, save to file
      cout<<"running clustering algorithm...";
      start = now();
      std::vector<shared_ptr<Hexel>> recClusters;
      algo->getRecClusters(recClusters, recHitsRaw);
      end = now();
      cout<<" done ("<<duration(start,end)<<" s)"<<endl;
      
      // get 3D clusters with HGCalAlgo
      cout<<"running clustering algorithm...";
      start = now();
      
      vector<vector<vector<unique_ptr<Hexel>>>> clusters;
      vector<shared_ptr<BasicCluster>> rec3Dclusters;
      algo->makeClusters(clusters, recHitsRaw);
      algo->make3DClusters(rec3Dclusters, clusters);
      end = now();
      cout<<" done ("<<duration(start,end)<<" s)"<<endl;

      // recClusters -> array of hexel objects
      cout<<"looking for hits associated with hexels...";
      start = now();
      vector<RecHits*> recHitsPerClusterArray;
      recHitsRaw->GetRecHitsPerHexel(recHitsPerClusterArray, recClusters);
      end = now();
      cout<<" done ("<<duration(start,end)<<" s)\n"<<endl;

      for(int index=0;index<rec3Dclusters.size();index++){
        cout<<"Multi-cluster (RE-RUN) index: "<<index;
        cout<<", No. of 2D-clusters = "<<rec3Dclusters[index]->GetHexelsPerLayer().size();
        cout<<", Energy  = "<<rec3Dclusters[index]->GetEnergy();
        cout<<", Phi = "<<rec3Dclusters[index]->GetPhi();
        cout<<", Eta = "<<rec3Dclusters[index]->GetEta();
        cout<<", z = "<<rec3Dclusters[index]->GetZ()<<endl;
      }
        
      
      auto endEvent = now();
      cout<<"Total event processing time: "<<duration(startEvent,endEvent)<<" s"<<endl;

      recHitsPerClusterArray.clear();
      if(!isTestBeam) simHitsPerClusterArray.clear();

    }
    delete tree;
    inFile->Close();
    delete inFile;
  }
  delete algo;

  return 0;
}
