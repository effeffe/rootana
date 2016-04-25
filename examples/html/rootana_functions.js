

// Promisified XLMHttpRequest wrapper...
// stolen from http://www.html5rocks.com/en/tutorials/es6/promises/
function getUrl(url, postData = false) {
  // Return a new promise.
  return new Promise(function(resolve, reject) {

    // Do the usual XHR stuff
    var req = new XMLHttpRequest();

    if(postData != false){
      req.open('POST', url);
    }else{
      req.open('GET', url);
    }


    req.onload = function() {
      // This is called even on 404 etc
      // so check the status
      if (req.status == 200) {
        // Resolve the promise with the response text
        resolve(req.response);
      }
      else {
        // Otherwise reject with the status text
        // which will hopefully be a meaningful error
        reject(req.statusText);
      }
    };

    // Handle network errors
    req.onerror = function() {
      reject("Network Error");
    };

    // Make the request
    if(postData != false){
      req.send(postData);
    }else{
      req.send();
    }
  });
}

// This variable keeps track of the webdirectory that the rootana THttpServer is posting to
rootana_dir = "";
//location.protocol + '//' + location.host + "/rootana_dnf/";
gFoundRootanaDir = false;
// This is the ROOT directory where we are looking for histograms.  Form is either 'rootana' or 'Files/somenameXXX.root'
active_directory = "";
// This is the full address to the current ROOT directory
histo_address = "";
// This is the list of objects in current ROOT directory.  This is a local copy of information that the rootana server provides.
gHistogramList = {};

// Global pointer to dygraph objects
// Currently supports 4 dygraph objects
gDygraphPointer = [0,0,0,0];

// Find the correct histogram in current ROOT directory list
function findHistogram(histogramName){
    

  // Try stripping a directory part of histogram name...
  histogramName2 = histogramName.split("/").pop();

  for(var j = 0; j < gHistogramList.length; j++){
    var object = gHistogramList[j];
    
    if(object["_name"] == histogramName){
      return object;      
    }
    // Check the DirectoryFile sub-directories too... 
    if(object["_kind"] == "ROOT.TDirectoryFile"){     
      for(var k = 0; k < object["_childs"].length; k++){
	var object2 = object["_childs"][k]; 
	if(object2["_name"] == histogramName2 ){
	  return object2;
	}
      }
    }                                                                                                                  
  }

  return false;
  
}


// Check the objects in a ROOT directory tree; return true if one of them is a TH1D
// Also check for sub-directories in file 
function check_for_histograms(subdir_tree){

  if(! '_child' in subdir_tree){
    document.getElementById("readstatus").innerHTML = "Don't have child" ;
    return false;
  }
  
  histList = []
  for(var j = 0; j < subdir_tree["_childs"].length; j++){
    var object = subdir_tree["_childs"][j];
    if(object["_kind"] == "ROOT.TH1D"){
      // Found a histogram object.
      document.getElementById("readstatus").innerHTML = "Have child with histo" + object["_name"] ;
      return true;
    }

    // Check the DirectoryFile sub-directories too...
    if(object["_kind"] == "ROOT.TDirectoryFile"){    
      for(var k = 0; k < object["_childs"].length; k++){                                                                             
	var object2 = object["_childs"][k];
	if(object2["_kind"] == "ROOT.TH1D"){ 
	  // Found a histogram object. 
	  document.getElementById("readstatus").innerHTML = "Have child with histo" + object2["_name"] ;
	  return true; 
	}          
      }
    }
  }
    
};

function parseRootDirectory(response){

    var rootStructureJSON;

    try {
      rootStructureJSON = JSON.parse(response);

    }
    catch(err) {
      document.getElementById("readstatus").innerHTML = "err " + err.message;
      return;
    }
    
  
    // Loop over the objects in child object, looking for either the rootana directory
    // or Files directory
    
    
    for(var i = 0; i < rootStructureJSON["_childs"].length; i++){
      var testDir = rootStructureJSON["_childs"][i];
      var name = testDir["_name"];
      
      // Check for histograms in the rootana directory
      if( name == "rootana"){
        
      }
      
      // Check for histograms in the Files directory
      if( name == "Files"){
        for(var j = 0; j < testDir["_childs"].length; j++){
          var fileDir = testDir["_childs"][j];
          var foundHistograms = check_for_histograms(fileDir);
          // If we found a valid ROOT directory, save information for later plotting
          if(foundHistograms){
            active_directory = "Files/" + fileDir["_name"];
            histo_address = rootana_dir  + active_directory;
            document.getElementById("readstatus").innerHTML = "Getting list of available histograms...";
            // Get the full list of histograms, so we can check them later
            gHistogramList = fileDir["_childs"];
            gFoundRootanaDir = true;
          }
        }
      }
    }

}


// This method will search the root directory structure.
// Start by looking for histograms in the rootana directory.
// If we don't find any histograms there, then look in the file directories.
// Can request to make the call asynchronously or synchronously; default is async with promise 
function find_active_root_directory(async = true){

  if(!async){

    // Get the JSON description of current ROOT directory
    var request = XMLHttpRequestGeneric();
    request.open('GET', rootana_dir + "h.json", false);
    request.send(null);
  
    if(request.status != 200){
      document.getElementById("readstatus").innerHTML = "Couldn't get basic ROOT structure back; status = " + request.status + ". Is rootana httpserver running?"; 
      document.getElementById("readstatus").style.color = 'red';
      return;
    }
    parseRootDirectory(request.responseText);

    return
  }


  // Wrap the XHR request in promise.
  getUrl(rootana_dir + "h.json").then(function(response) {

    parseRootDirectory(response);

  }).catch(function(error) { // Handle exception if we didn't find ROOT structure

    document.getElementById("readstatus").innerHTML = "ASYNC Couldn't get basic ROOT structure back; status = " + error + ". Is rootana httpserver running?";   
    document.getElementById("readstatus").style.color = 'red';

  });
}

function rootanaResetHistogram(histogramName){

  // Find the directory structure, if it is not yet found.
  if(!gFoundRootanaDir){
    find_active_root_directory();
  }

  // Send the request for data for this plot
  var request = XMLHttpRequestGeneric();
  request.open('GET', histo_address + "/" + histogramName +"/exe.json?method=Reset", false);
  request.send(null);
  if(request.status != 200){ 
    document.getElementById("readstatus").innerHTML = "Failed to reset histogram " + histogramName;
    document.getElementById("readstatus").style.color = 'red';
    return false;
  }
  document.getElementById("readstatus").innerHTML = "Reset histogram " + histogramName;

  return true;

}

function rootanaResetAllHistograms(){

  for(var j = 0; j < gHistogramList.length; j++){
    var object = gHistogramList[j];
    
    rootanaResetHistogram(object["_name"]);

  }
}


gPlotlyPointer = 0;

function makePlot2D(divName, histoObject,histoInfoJSON, dygraphIndex,deleteDygraph){
  
  var title = histoObject["_title"];
      
  // Need to recalculate the bin centers.
  var bin_widthX = (histoInfoJSON["fXaxis"]["fXmax"] - histoInfoJSON["fXaxis"]["fXmin"]) / histoInfoJSON["fXaxis"]["fNbins"];
  var bin_widthY = (histoInfoJSON["fYaxis"]["fYmax"] - histoInfoJSON["fYaxis"]["fYmin"]) / histoInfoJSON["fYaxis"]["fNbins"];

  var x = [];
  var y = [];
  var z = new Array(histoInfoJSON["fYaxis"]["fNbins"]);
  for (var i=0; i<histoInfoJSON["fYaxis"]["fNbins"]; i++){
    z[i] = new Array(histoInfoJSON["fXaxis"]);
  }
      
  // Urgh, plot.ly seems to define arrays opposite to ROOT
  for (var iyy = 1; iyy <= histoInfoJSON["fYaxis"]["fNbins"]; iyy++){
    for (var ixx = 1; ixx <= histoInfoJSON["fXaxis"]["fNbins"]; ixx++){
      var index = iyy*(histoInfoJSON["fXaxis"]["fNbins"] +2) + ixx;
      z[iyy-1][ixx-1] = Number(histoInfoJSON["fArray"][index]);
    }
  }


  //document.getElementById("readstatus").innerHTML = z;
  var data = [
              {
                z: z,
                type: 'heatmap'
              }
              ];
  var layout = {
    title: histoObject["_title"],
    xaxis: {
      title: histoInfoJSON["fXaxis"]["fTitle"],
    },
    yaxis: {
      title: histoInfoJSON["fYaxis"]["fTitle"],
    },
    showlegend: false
  };

  //  delete gDygraphPointer[dygraphIndex] ;
  if(gDygraphPointer[dygraphIndex]){
    gDygraphPointer[dygraphIndex].destroy();
    gDygraphPointer[dygraphIndex] = 0;
  }
  //if(gPlotlyPointer) delete gPlotlyPointer;
  //gd2 = document.getElementById(divName);
  //purge(gd2);

  Plotly.newPlot(divName, data,layout);

}


function makePlot1D(histoInfoJSONFirst,plotType,divName,csv_array,deleteDygraph,dygraphIndex){  

  var title = histoInfoJSONFirst["fName"];
  if(plotType == "overlay"){
    title = "Overlay Histograms";
  }
  
  //  g = new Dygraph(document.getElementById(divName),csv_array_multi,{title: 'multiple histograms'});
  if(deleteDygraph || gDygraphPointer[dygraphIndex] == 0){
    delete gDygraphPointer[dygraphIndex] ;
    gDygraphPointer[dygraphIndex]  = new Dygraph(document.getElementById(divName),csv_array,
                                                 {title: title, 'labelsSeparateLines' : true, 'legend' : 'always', ylabel : histoInfoJSONFirst["fYaxis"]["fTitle"], xlabel : histoInfoJSONFirst["fXaxis"]["fTitle"]});
  }else{
    gDygraphPointer[dygraphIndex].updateOptions( { 'file': csv_array} );
  }
  document.getElementById("readstatus").innerHTML = "Rootana data correctly read";
  document.getElementById("readstatus").style.color = 'black';

}



// This function handles making a CSV object for use in dygraph 1D plot...
// The process for making the CSV is different for a single plot vs overlay plot.
function makeCSVArray(plotType,histoInfoJSON,histoObject,dataIndex = 0){

  if(plotType == "single" || plotType == "multiple"){
      
    // Fill the CSV array to make the histogram.

    // Need to recalculate the bin centers.
    var bin_width = (histoInfoJSON[dataIndex]["fXaxis"]["fXmax"] - histoInfoJSON[dataIndex]["fXaxis"]["fXmin"]) / histoInfoJSON[dataIndex]["fXaxis"]["fNbins"]
    var csv_array = histoInfoJSON[dataIndex]["fXaxis"]["fTitle"] + ", " + histoInfoJSON[dataIndex]["fYaxis"]["fTitle"] + "\n";

    for (i = 0; i < histoInfoJSON[dataIndex]["fXaxis"]["fNbins"]; i++){
      // remember, we skip the first bin of data, since it is the underflow bin...
      var bin_center = (i*bin_width) + bin_width/2.0;
      csv_array += bin_center + "," +   histoInfoJSON[dataIndex]["fArray"][i+1] + "\n";    
    }
    return csv_array;
    
  }else{ // handle consolidating data in N-dimensional CSV array...

    histoNames = [];
    histoValues = [];
    histoBinCenters = [];
    histoXTitle = ""


    // loop over all the results, consolidating them into CSV
    var title = histoObject["_title"];
    for(var index = 0; index < histoInfoJSON.length; index++){
      histoNames.push(histoInfoJSON[index]["fTitle"]);
      
      // Need to recalculate the bin centers.
      var bin_width = (histoInfoJSON[index]["fXaxis"]["fXmax"] - histoInfoJSON[index]["fXaxis"]["fXmin"]) / histoInfoJSON[index]["fXaxis"]["fNbins"];
      
      if(index == 0)
        histoXTitle = histoInfoJSON[index]["fXaxis"]["fTitle"]
      arrayValues = [];
      for (i = 0; i < histoInfoJSON[index]["fXaxis"]["fNbins"]; i++){
        // remember, we skip the first bin of data, since it is the underflow bin...
        var bin_center = (i*bin_width) + bin_width/2.0;

        // same the bin centers using first histogram
        if(index == 0){ histoBinCenters.push(bin_center);}      
        arrayValues.push(histoInfoJSON[index]["fArray"][i+1]);
      }
    
      histoValues.push(arrayValues);

    }

    // Create the CVS array for the combined information.
    var csv_array_multi = histoXTitle;
    for(var ii = 0; ii < histoNames.length; ii++){
      csv_array_multi += ", " + histoNames[ii];
    }
    csv_array_multi += "\n";
    for(var ii = 0; ii < histoValues[0].length; ii++){
      csv_array_multi += histoBinCenters[ii];
      for(var jj = 0; jj < histoValues.length; jj++){
        csv_array_multi += ", " + histoValues[jj][ii];
      }
      csv_array_multi += "\n";    
    }

    return csv_array_multi;
    
  }

}

var promiseAlreadyInFligth = false;

// This method will create a plot in the requested 
// divs using the requested histograms.
// We use a multi.json to get the data for all the histograms we want to plot;
// we wrap this in a promise to provide async response.
// Then handle the data we get back differently for single histogram vs overlay vs multiple,
// as well as handling 1D vs 2D...
function plotAllHistograms(plotType,divNames, histogramNameList, deleteDygraph){

  // Don't make another request if last request isn't finished.
  if(promiseAlreadyInFligth) return;
  promiseAlreadyInFligth = true;

  
  // Wrap the request in promise; will combine multiple items (if there are more than 1) into single XHR request
  var listDirectories = "";
  for(var index = 0; index < histogramNameList.length; index++){
    var name = active_directory + "/" + histogramNameList[index];
    listDirectories += name + "/root.json\n";
  }    
  console.log(listDirectories);
  // Make the promise XHR
  var url = rootana_dir + "multi.json?number="+String(histogramNameList.length);
  getUrl(url, listDirectories).then(function(response) {

    promiseAlreadyInFligth = false;
    var histoInfoJSON = JSON.parse(response);
    
    // Check that we can find this histogram in current directory list
    if(histoObject == false){
      document.getElementById("readstatus").innerHTML = "Failed to find histogram with name " 
        + histogramName + " in current ROOT directory";
      document.getElementById("readstatus").style.color = 'red';
  
      return;
    }

    // Loop over the divs
    for(var index = 0; index < divNames.length; index++){
            
      var histoObject = findHistogram(histoInfoJSON[0]["fName"]);
      
      // handle 1D vs 2D histograms differently; check first histogram for type
      if(histoObject["_kind"].search("ROOT.TH2") != -1){
        
        // Make the plot-ly 2D plot
        makePlot2D(divNames[index], histoObject,histoInfoJSON[index], index,deleteDygraph);
        
      }else if(histoObject["_kind"].search("ROOT.TH1") != -1){
        
        // Create the CSV array; this happens differently for single/multiple vs overlay histograms.
        var csv_array = makeCSVArray(plotType,histoInfoJSON,histoObject,index);
        // Make the dygraph 1D plot
        makePlot1D(histoInfoJSON[index],plotType,divNames[index],csv_array,deleteDygraph,index)
        
      }else{
        
        document.getElementById("readstatus").innerHTML = "Histogram with name " 
          + histoInfoJSON[0]["fName"] + " is not TH1 or TH2... don't know how to handle.";
        document.getElementById("readstatus").style.color = 'red';
        
      }
               
    }

    
  }).catch(function(error) { // Handle exception if we didn't find the histogram...
    
    document.getElementById("readstatus").innerHTML = "Couldn't get histogram data; status = " + error + ". Did rootana httpserver die?";
    document.getElementById("readstatus").style.color = 'red';    
    // If we couldn't find histogram, try forcing re-find of rootana directory
    gFoundRootanaDir = false;
  });

}
