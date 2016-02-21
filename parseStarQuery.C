#include "Riostream.h"
#include "TROOT.h"
#include "Rtypes.h"
#include "TNamed.h"
#include "TString.h"
#include "TList.h"
#include "TFile.h"
#include "TTimeStamp.h"
#include "TObjArray.h"
#include "TObjString.h"

#include <vector>
#include <map>

//#include "nodeClass.C"

#include "Riostream.h"
#include "Rtypes.h"
#include "TNamed.h"
#include "TString.h"
#include "TList.h"
#include "TFile.h"
#include "TTimeStamp.h"
#include "TObjArray.h"
#include "TObjString.h"

#if 1
// -- water marks for the coloring 
static const ULong64_t gcLowMark  = 1099511627776;
static const ULong64_t gcHighMark = 3*1099511627776;

static const Int_t     gcUid[4]            = {000, 001, 002, 003};
static const Int_t     gcGid[4]            = {000, 001, 002, 003};

// -- input files
static const Char_t*   gcFolder[2]         = {"alice", "rnc"};
static const Char_t*   gcStorage[2]        = {"project", "projecta"}; 
static const Char_t*   gcStoragePrefix[2]  = {"prj2", "prjA"}; 

static const Char_t*   gcProjectFolder[3]  = {"alice", "star", "starprod"};

static const Int_t     gcMaxFiles[3]       = {2, 2, 4};

// -- max depth to print nodes po
static const Int_t     gcMaxLevel = 6;

// -- current storage folder
static TString         gStorage("");

// _______________________________________________________________
class node : public TNamed  {

public:
  // ___________________________________________________
  node() :
    fLevel(0),
    fMaxLevel(gcMaxLevel),
    fOwnSize(0),
    fChildSize(0),
    fNOwnFiles(0),
    fNChildFiles(0),
    fUid(-1),
    fGid(-1),
    faTime(-1.), 
    fcTime(2145916800),
    fmTime(-1.), 
    fStorage(gStorage),
    fChildren(NULL) {
    // -- default constructor
    
    SetNameTitle("root", "root");
    fChildren = new TList();
    fChildren->SetOwner(kTRUE);
  }
  
  // ___________________________________________________
  node(TString name, TString title, Int_t level) :
    fLevel(level),
    fMaxLevel(gcMaxLevel),
    fOwnSize(0),
    fChildSize(0),
    fNOwnFiles(0),
    fNChildFiles(0),
    fUid(-1),
    fGid(-1),
    faTime(-1),
    fcTime(2145916800),
    fmTime(-1),
    fStorage(gStorage),
    fChildren(NULL) {
    // -- constructor for adding file only

    SetNameTitle(name, title);
    fChildren = new TList();
    fChildren->SetOwner(kTRUE);
  }

  // ___________________________________________________
  virtual ~node() {
    // -- destructor
    
    if (fChildren) {
      fChildren->Clear();
      delete fChildren;
    }
  }
  
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Setter
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  // ___________________________________________________
  void CopyProperties(node* orig) {
    // -- Copy properties of a node
    SetProperties(orig->GetOwnSize(), orig->GetChildSize(), 
		  orig->GetNOwnFiles(), orig->GetNChildFiles(), 
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void CopyPropertiesOwn(node* orig) {
    // -- Copy properties of a node
    SetProperties(orig->GetOwnSize(), 0, 
		  orig->GetNOwnFiles(), 0,
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void CopyPropertiesChild(node* orig) {
    // -- Copy properties of a node
    SetProperties(0, orig->GetSize(), 
		  0, orig->GetNFiles(), 
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void SetProperties(const ULong64_t sizeOwn, const ULong64_t sizeChild, const Int_t nFilesOwn, const Int_t nFilesChild, 
		     const Int_t aTime, const Int_t cTime, const Int_t mTime) { 
    // -- Set node properties

    // -- add size and nFiles
    fOwnSize += sizeOwn; 
    fChildSize += sizeChild; 

    fNOwnFiles += nFilesOwn;
    fNChildFiles += nFilesChild;

    // -- use last acess time in folder
    if (aTime > faTime) faTime = aTime;

    // -- use last modification time in folder
    if (mTime > fmTime) fmTime = mTime;

    // -- use first creation time in folder
    if (cTime < fcTime) fcTime = cTime;
  }
  
  // ___________________________________________________
  void SetMaxLevel(Int_t maxLevel) {
    // -- Set MaxLevel to all children (if needed)
    
    fMaxLevel = maxLevel;
    
    if (fLevel <= fMaxLevel) {
      TIter next(fChildren);
      node *child;
      
      while ((child = static_cast<node*>(next())))
	child->SetMaxLevel(maxLevel);
    }
  }

  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Getter
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  ULong64_t     GetOwnSize()     { return fOwnSize; }
  ULong64_t     GetChildSize()   { return fChildSize; }
  ULong64_t     GetSize()        { return fOwnSize+fChildSize; }

  Int_t         GetNOwnFiles()   { return fNOwnFiles; }
  Int_t         GetNChildFiles() { return fNChildFiles; }
  Int_t         GetNFiles()      { return fNOwnFiles+fNChildFiles; }

  TList*        GetChildren()    { return fChildren; }
  node*         GetChild(const Char_t* childName) { return static_cast<node*>(fChildren->FindObject(childName)); }

  Int_t         GetaTime()      { return faTime; }
  Int_t         GetcTime()      { return fcTime; }
  Int_t         GetmTime()      { return fmTime; }

  const Char_t* GetaDate()      { return GetDate(faTime); }
  const Char_t* GetcDate()      { return GetDate(fcTime); }
  const Char_t* GetmDate()      { return GetDate(fmTime); }


  // ___________________________________________________
  void ClearChild(const Char_t* childName) {  
    // -- clear specific child
    
    TString sChildName(childName);
    sChildName.ToLower();

    if (!fChildren)
      return;
    
    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next()))) {
      if (!child)
	continue;
      if (!sChildName.CompareTo(child->GetName())) {
	child->ClearChilds();
	cout << "CLEARED  childs  " << child->GetName() << endl;
	if (child) {
	  delete child;
	  child = NULL;
	}
	cout << " done " << endl;
      }
    }
  }

  // ___________________________________________________
  void ClearChilds() {  
    // -- clear children
    
    cout << "NAME   " << GetName() << endl;

    if (fChildren) {
      
      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next())))
	child->ClearChilds();
      cout << "Delete   " << GetName() << endl;
      fChildren->Delete();
	cout << "Delete Done  " << GetName() << endl;
    }
  }

  // ___________________________________________________
  const Char_t* GetDate(UInt_t date) {
    // -- get a human and machine sortable date

    UInt_t year, month, day;

    TTimeStamp timeStamp(date);
    timeStamp.GetDate(0, 0, &year, &month, &day);

    return Form("%d-%02d-%02d", year, month, day);
  }

  // ___________________________________________________
  const Char_t* GetHumanReadableSize() {
    // -- get a human readable size
    
    const Char_t* sizeOrder[] = {"", "k", "M", "G", "T", "P", "E"};

    Int_t idx = 0;
    Double_t size = Double_t(GetSize());
      
    while (1) {
      Double_t tmpSize = size/1024.0;
      if (tmpSize < 1)
	return Form("%.2f%sB", size, sizeOrder[idx]);
      size = tmpSize;
      ++idx;
    }
  }

  // ___________________________________________________
  const Char_t* GetGBSize() {
    // -- get a human readable size in GByte
    return (GetSize() < 5*1024*1024) ? Form("&lt;0.001") : Form("%.2f",  Double_t(GetSize())/(1024.0*1024.0*1024.0));;
  }

  // ___________________________________________________
  const Char_t* GetAlarmLevel() {
    // -- print alarm level

    if (GetSize() <= gcLowMark)
      return "normal";
    else 
      return (GetSize() > gcHighMark) ? "alarm" : "warning";
  }
  
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Add methods
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  // ___________________________________________________
  void AddFile(TString &title, ULong64_t size, Int_t aTime, Int_t cTime, Int_t mTime) { 
    // -- add file into tree

    // -- find position of '/'
    Short_t first = title.First('/');

    // -- if leaf  ( no '/' inside)
    //      add own size + child && return
    //    else
    //      add child
    if (first < 0)  {
      SetProperties(size, 0, 1, 0, aTime, cTime, mTime);
      return;
    }
    else 
      SetProperties(0, size, 0, 1, aTime, cTime, mTime);

    // -- current path
    TString current(title(0, first));
    
    // -- remaining path - without current
    title.Remove(0,first+1);
    
    // -- get child node, if no exist -> create it
    node* child = AddNode(current);

    // -- add recursively the child till at the leaf
    child->AddFile(title, size, aTime, cTime, mTime);
  }

  // ___________________________________________________
  node* AddNode(TString &title) {
    // -- Add node to the tree, return if already exists
    
    TString name(title);
    name.ToLower();
    
    // -- check if child exists
    node* child = GetChild(name);
    if (!child) {
      fChildren->Add(new node(name, title, fLevel+1));
      child = static_cast<node*>(fChildren->Last());
    }

    return child;
  }
  
  // ___________________________________________________
  node* AddNode(const Char_t* titleT) {
    // -- Add node to the tree, return if already exists

    TString title(titleT);
    return AddNode(title);
  }
  
  // ___________________________________________________
  node* AddNodeCopy(node* orig, const Char_t* newTitle = "") {
    // -- add a copy of a node
    //    NOT of its children
    
    TString title(newTitle);
    if (title.Length() == 0)
      title += orig->GetTitle();
  
    // -- get new node or already existing
    node* copy = AddNode(title);
    
    // -- add the properties
    copy->CopyPropertiesOwn(orig);
    
    return copy;
  }

  // ___________________________________________________
  node* AddNodeFullCopy(node* orig, const Char_t* newTitle = "") {
    // -- add a copy of a node
    //    NOT of its children
    
    TString title(newTitle);
    if (title.Length() == 0)
      title += orig->GetTitle();
  
    // -- get new node or already existing
    node* copy = AddNode(title);
    
    // -- add the properties
    copy->CopyProperties(orig);
    
    // -- merge the children 
    copy->AddChildren(orig);

    return copy;
  }

  // ___________________________________________________
  void AddChildren(node* orig) {
    // -- Add children from list 

    // -- loop over foreign children list and 
    //    - create a full copy of foreign nodes
    //    - otherwise merge 
    TIter next(orig->GetChildren());
    node *childOrig;
    while ((childOrig = static_cast<node*>(next())))
      AddNodeFullCopy(childOrig);
      SumAddChildren();
  }
  
  // ___________________________________________________
  void SumAddChildren() {
    // -- Sum and add children to current node

    fChildSize   = 0;
    fNChildFiles = 0;

    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next())))
      CopyPropertiesChild(child);
  }

  // ___________________________________________________
  void PrintNodes(ofstream &fout) { 
    // -- print node recursively
    PrintNodes(fout, fLevel+1);
  }

  // ___________________________________________________
  void PrintNodes(ofstream &fout, Int_t level) { 
    // -- print node recursively

    // -- Re-adjusting the level
    fLevel = level;
  
    TString padding("");
    for (Int_t idx = 0; idx < fLevel; ++idx)
      padding += "     ";

    fout << padding << " { label: '" << GetTitle() << " [<span class=\"nFiles\">" << GetNFiles() 
	 << " files</span>] <span class=\"size " << GetAlarmLevel() << "\">" << GetHumanReadableSize() 
	 << "</span>&nbsp;&nbsp;&nbsp;<span class=\"lastMod\"> {Last Mod. " << GetmDate() << "}</span>'," << endl;

    if (fLevel < fMaxLevel && fChildren->GetEntries() > 0) {
      fout << padding << "   children: [" << endl;

      fChildren->Sort();

      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next())))
	child->PrintNodes(fout, fLevel+1);

      fout << padding << "   ]," << endl;
    }
    fout << padding << " }," << endl;
  }  

  // ___________________________________________________
  void PrintChildren(Int_t maxLevel = 0, Int_t currentLevel = 0) {
    // -- print names of children down to maxLevel
    
    TString padding("");
    for (Int_t idx = 0; idx < currentLevel; ++idx)
      padding += "     ";
    
    fChildren->Sort();
    
    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next()))) {
      cout <<  padding << child->GetTitle() 
	   << " || " <<  child->GetNFiles()
	   << " || " <<  child->GetSize()
	   << " || " <<  child->GetHumanReadableSize() << endl;

      if (currentLevel != maxLevel)
	child->PrintChildren(maxLevel, currentLevel+1);
    }
  }
  
  // ___________________________________________________
  void PrintTableEntries(ofstream &fout, Int_t version, const Char_t *arg1 = "") { 
    // -- print childs as table entry
    //    use extended format if arg1 is none empty
    //    version : 1 user || embedding
    //              2 user + storage
    //              0 extended embedding

    if (fChildren->GetEntries() > 0) {
      fChildren->Sort();
      
      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next()))) {
	fout << "<tr>";
	
	if (version == 1) {
	  fout << "<td class=\"user\">" << child->GetTitle() << "</td>";
	}
	else if (version == 2) {
	  fout << "<td class=\"user\">" << child->GetTitle() << "</td>";
	  fout << "<td class=\"user\">" << GetTitle() << "</td>";
	}
	else if (version == 0) {
	  fout << "<td class=\"user\">" << arg1 << "</td>"
	       << "<td class=\"user\">" << child->GetTitle() << "</td>";
	  fout << "<td class=\"user\">" << GetTitle() << "</td>";
	}
	  
	fout << "<td class=\"size " << child->GetAlarmLevel() << "\">" << child->GetGBSize() << "</td>" 
	     << "<td class=\"nFiles\">" << child->GetNFiles() << "</td>"
	     << "<td class=\"time\">" << child->GetcDate() << "</td>"
	     << "<td class=\"time\">" << child->GetmDate() << "</td>"
	     << "<td class=\"time\">" << child->GetaDate() << "</td></tr>" << endl;
      } // while ((child = static_cast<node*>(next()))) {
    } // if (fChildren->GetEntries() > 0) {
  }

  // ___________________________________________________
  void PrintTableSummary(ofstream &fout, Int_t extraCols = 0) { 
    // -- print childs as table entry
    
    fout << "<tr><td class=\"user footer\" style=\"font-weight: bold;\">TOTAL</td>";
    
    for (Int_t idx = 0; idx < extraCols; ++idx) 
      fout << "<td class=\"user footer\">&nbsp</td>";
    
    fout << "<td class=\"size footer " << GetAlarmLevel() << "\">" << GetGBSize() << "</td>" 
	 << "<td class=\"nFiles footer\">" << GetNFiles() << "</td>"
	 << "<td class=\"time footer\">" << GetcDate() << "</td>"
	 << "<td class=\"time footer\">" << GetmDate() << "</td>"
	 << "<td class=\"time footer\">" << GetaDate() << "</td></tr>" << endl;
  }  
  
private:
  Int_t     fLevel;
  Int_t     fMaxLevel;

  ULong64_t fOwnSize;
  ULong64_t fChildSize;
  Int_t     fNOwnFiles;
  Int_t     fNChildFiles;
  
  Int_t     fUid;
  Int_t     fGid;

  Int_t     faTime;
  Int_t     fcTime;
  Int_t     fmTime;

  TString   fStorage;

  TList*    fChildren;

  ClassDef(node,1)
};
#endif


// ________________________________________________________________________________
void parseStarQuery() {

  //  gROOT->LoadMacro("nodeClass.C++");

  TString inFileName("test.all");

  // -- open input file
  ifstream fin(inFileName);
  if (!fin.good()) {
    printf ("File %s couldn't be opened!", inFileName.Data());
    return; 
  }
   
  // --------------------------------------------------------------------

  vector<string> vFiles;
  
  node *root = new node;

  Int_t idxDataSet = 0;

  // -- Loop of file - line-by-line
  while (1) {
    string line;
    
    // -- read in	
    getline (fin, line);
    
    // -- break at at of file
    if (fin.eof()) {
      printf("Processed %d datasets of file %s\n", idxDataSet, inFileName.Data());
      break;
    }
    
    // -- break if error occured during reading
    if (!fin.good()) {
      printf("Error after processing %d datasets of file %s\n", idxDataSet, inFileName.Data());
      break;
    }

    // -- new dataSet
    // ----------------------------------------------
    if (!line.compare("{")) {
      ++idxDataSet;
      
      TString   path;
      ULong64_t size;

      // -- Loop dataSet
      while (1) {
	// -- read in	
	string setLine;
	getline (fin, setLine);

	// -- end dataSet
     	if (!setLine.compare("}"))  {
	  
	  // -- check if already in - otherwise add
	  if (std::find(vFiles.begin(), vFiles.end(), path.Data()) == vFiles.end()) {
	    vFiles.push_back(path.Data());

	    // -- Addline
	    root->AddFile(path, size, 0, 0, 0);
	  }
      	  break;
	}

	// --------------------------------------
	TString sLine(setLine);

	TObjArray *tokenizedLine = sLine.Tokenize(":");
        if (!tokenizedLine) 
	  printf("Error tokenizing line %d: %s\n", idxDataSet, sLine.Data());
        else {
	  TString item((static_cast<TObjString*>(tokenizedLine->At(0)))->String());
	  TString value((static_cast<TObjString*>(tokenizedLine->At(1)))->String());
	  
	  item.ReplaceAll('"', "");
 	  item.ReplaceAll(' ', "");

	  if (!item.CompareTo("fullpath") || !item.CompareTo("size")) {
	    value.ReplaceAll('"', "");
	    value.ReplaceAll(' ', "");
	    value.ReplaceAll(',', "");

	    if (!item.CompareTo("fullpath"))
	      path = value.Strip(TString::kLeading,'/');
	    else if (!item.CompareTo("size")) 
	      size = value.Atoi();
	  }
	}
	
	tokenizedLine->Clear();
	delete tokenizedLine;
      }
    }
    
    // -- print info on status
    if (!(idxDataSet%1000))
      printf("Processing dataSet %d of file %s\n", idxDataSet, inFileName.Data());
  }

  fin.close();

  // -------------------------------------------------------------------------

  cout << endl << " ------------------------------------------------------------- " << endl << endl;  

  node *pico = root->GetChild("star")->GetChild("picodsts");
  pico->GetChildren()->Sort();
  
  TIter nextRun(pico->GetChildren());
  node *run;
  while ((run = static_cast<node*>(nextRun()))) {
    run->GetChildren()->Sort();
    
    TIter nextSystem(run->GetChildren());
    node *system;
    while ((system = static_cast<node*>(nextSystem()))) {
      system->GetChildren()->Sort();

      TIter nextEnergy(system->GetChildren());
      node *energy;
      while ((energy = static_cast<node*>(nextEnergy()))) {
	energy->GetChildren()->Sort();	

	TIter nextTrigger(energy->GetChildren());
	node *trigger;
	while ((trigger = static_cast<node*>(nextTrigger()))) {
	  trigger->GetChildren()->Sort();

	  TIter nextProduction(trigger->GetChildren());
	  node *production;
	  while ((production = static_cast<node*>(nextProduction()))) {
	    production->GetChildren()->Sort();

	    cout << run->GetTitle() << " || " 
		 << system->GetTitle() << " || " 
		 << energy->GetTitle() << " || " 
		 << trigger->GetTitle() << " || " 
		 << production->GetTitle() << " || " 
		 << production->GetNFiles() << " || " 
		 << production->GetSize() << " || " 
		 << production->GetHumanReadableSize() << endl;
	    
	  } // while ((production = static_cast<node*>(nextProduction()))) {  
	} // while ((trigger = static_cast<node*>(nextTrigger()))) {
      } // while ((energy = static_cast<node*>(nextEnergy()))) {
    } // while ((system = static_cast<node*>(nextSystem()))) {
  } // while ((run = static_cast<node*>(nextRun()))) {

  cout << endl << " ------------------------------------------------------------- " << endl << endl;

  pico->PrintChildren(4);

  cout << endl << " ------------------------------------------------------------- " << endl << endl;

  // -------------------------------------------------------------------------

  root->PrintChildren(10);

  // -------------------------------------------------------------------------
  // -- Save Parsed Tree
  TFile* outFile = TFile::Open("treeOutput.root", "RECREATE");
  if (outFile) {
    outFile->cd();
    root->Write();
    outFile->Close();
  }
}
