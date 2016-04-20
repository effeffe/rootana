

// Promisified XLMHttpRequest wrapper...
// stolen from http://www.html5rocks.com/en/tutorials/es6/promises/
function getUrl(url) {
  // Return a new promise.
  return new Promise(function(resolve, reject) {

    // Do the usual XHR stuff
    var req = new XMLHttpRequest();
    req.open('GET', url);

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
    req.send();
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


function fillHistogranPlot1D(divName, histoObject,histoInfoJSON, dygraphIndex,deleteDygraph){

  var title = histoObject["_title"];
      
  // Fill the CSV array to make the histogram.
  // Need to recalculate the bin centers.
  var bin_width = (histoInfoJSON["fXaxis"]["fXmax"] - histoInfoJSON["fXaxis"]["fXmin"]) / histoInfoJSON["fXaxis"]["fNbins"]
    var csv_array = "ADC value, Number Entries\n";
  for (i = 0; i < histoInfoJSON["fXaxis"]["fNbins"]; i++){
    // remember, we skip the first bin of data, since it is the underflow bin...
    var bin_center = (i*bin_width) + bin_width/2.0;
    csv_array += bin_center + "," +   histoInfoJSON["fArray"][i+1] + "\n";    
  }
  //alert("DDDDoing this index " + divName + " " + dygraphIndex)
  if(deleteDygraph || gDygraphPointer[dygraphIndex] == 0){
    delete gDygraphPointer[dygraphIndex] ;
    gDygraphPointer[dygraphIndex]  = new Dygraph(document.getElementById(divName),csv_array,{title: title, 'labelsSeparateLines' : true, 'legend' : 'always'});
    //alert("Doing this index " + divName + " " + dygraphIndex)
  }else{
    gDygraphPointer[dygraphIndex].updateOptions( { file: csv_array} );
  }

}

gPlotlyPointer = 0;

function fillHistogranPlot2D(divName, histoObject,histoInfoJSON, dygraphIndex,deleteDygraph){
  
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


// Make a separate check for each graph, since we don't want to clobber a
// request for a different div...
var promiseInflight = [false,false,false,false];
// This method will create a dygraph plot in the requested 
// div for the requested histogram.
function fillHistogramPlot(divName, histogramName, dygraphIndex, deleteDygraph){
  
	// Ignore this request if a previous promise has not yet been returned...
	if(promiseInflight[dygraphIndex]){
			//console.log("ignore... in flight " + histogramName);
   return;
  } 

  // Find the directory structure, if it is not yet found.
  if(!gFoundRootanaDir){
    find_active_root_directory();
    // return this time, since the directory structure won't be defined in time for this execution (async request)
    return;
  }

  promiseInflight[dygraphIndex] = true;
  // Wrap the request in promise.
  getUrl(histo_address + "/" + histogramName +"/root.json.gz?compact=3").then(function(response) {

    promiseInflight[dygraphIndex] = false;
 
    var histoInfoJSON = JSON.parse(response);
    
    // Check that we can find this histogram in current directory list
    var histoObject = findHistogram(histogramName);
    if(histoObject == false){
      document.getElementById("readstatus").innerHTML = "Failed to find histogram with name " 
					+ histogramName + " in current ROOT directory" + gHistogramList;
      document.getElementById("readstatus").style.color = 'red';    
			return;
    }
        
    // Handle either TH1 or TH2... otherwise don't know how to plot it...
    if(histoObject["_kind"].search("ROOT.TH1") != -1){
      fillHistogranPlot1D(divName, histoObject,histoInfoJSON, dygraphIndex,deleteDygraph);
    }else if(histoObject["_kind"].search("ROOT.TH2") != -1){
      fillHistogranPlot2D(divName, histoObject,histoInfoJSON, dygraphIndex,deleteDygraph);
    }else{
      document.getElementById("readstatus").innerHTML = "Overlaid histogram with name " 
        + histogramName + " is not TH1... don't know how to handle.";
      document.getElementById("readstatus").style.color = 'red';      
      return;
    }

    document.getElementById("readstatus").innerHTML = "Rootana data correctly read";
    document.getElementById("readstatus").style.color = 'black';
    
  }).catch(function(error) { // Handle exception if we didn't find the histogram...
    
    document.getElementById("readstatus").innerHTML = "Couldn't get histogram data; status = " + error + ". Did rootana httpserver die?";
    document.getElementById("readstatus").style.color = 'red';    
    console.error('Augh, there was an error!', error);
    // If we couldn't find histogram, try forcing re-find of rootana directory
    gFoundRootanaDir = false;
  });
      
}

// This method will create a dygraph plot in the requested 
// div for each of the requested histogram in list.
// use chained promises.
function fillMultipleHistogramPlot(divName, histogramNameList, dygraphIndex, deleteDygraph){

  // arrays for storing the bin contents
  histoNames = [];
  histoValues = [];
  histoBinCenters = [];
  histoXTitle = ""

  for(var index = 0; index < histogramNameList.length; index++){
    histogramName = histogramNameList[index];

    // Send the request for data for this plot
    // Find the directory structure, if it is not yet found.
    if(!gFoundRootanaDir){
      find_active_root_directory();
    }
    
    
    // Send the request for data for this plot
    var histoInfoJSON;  
    var request = new XMLHttpRequest();
    request.open('GET', histo_address + "/" + histogramName +"/root.json.gz?compact=3", false);
    request.send(null);
    if(request.status != 200){ 
      
      // try re-reading the directory tree... see if that allows us to find the histogram...
      find_active_root_directory();
      var request2 = new XMLHttpRequest();
      request2.open('GET', histo_address + "/" + histogramName +"/root.json.gz?compact=3", false);
      request2.send(null);
      if(request2.status != 200){ 
        document.getElementById("readstatus").innerHTML = "Couldn't get histogram data; status = " + request.status + ". Did rootana httpserver die?";
        document.getElementById("readstatus").style.color = 'red';
        return;
      }
      
      histoInfoJSON = JSON.parse(request2.responseText);
    }
    
    histoInfoJSON = JSON.parse(request.responseText);
    
    // Check that we can find this histogram in current directory list
    var histoObject = findHistogram(histogramName);
    if(histoObject == false){
      document.getElementById("readstatus").innerHTML = "Failed to find histogram with name " 
        + histogramName + " in current ROOT directory";
      document.getElementById("readstatus").style.color = 'red';
  
      return;
    }

    // Check that this is a TH2... otherwise don't know how to plot it...
    if(histoObject["_kind"].search("ROOT.TH2") != -1){
      document.getElementById("readstatus").innerHTML = "Can't meaningfully overlay 2D histogram. Not plotting. ";
      document.getElementById("readstatus").style.color = 'orange';      
      // delete old plot first
      if(deleteDygraph || gDygraphPointer[dygraphIndex] == 0){
        //delete gDygraphPointer[dygraphIndex];
        gDygraphPointer[dygraphIndex].destroy();
        
      }
      return;
    }    
    // Check that this is a TH1... otherwise don't know how to plot it...
    if(histoObject["_kind"].search("ROOT.TH1") == -1){
      document.getElementById("readstatus").innerHTML = "Histogram with name " 
        + histogramName + " is not TH1... don't know how to handle.";
      document.getElementById("readstatus").style.color = 'red';
      
      return;
    }
    var title = histoObject["_title"];

    
    // Fill the CSV array to make the histogram.
    // Need to recalculate the bin centers.
    var bin_width = (histoInfoJSON["fXaxis"]["fXmax"] - histoInfoJSON["fXaxis"]["fXmin"]) / histoInfoJSON["fXaxis"]["fNbins"];
    var csv_array = "ADC value, Number Entries\n";
    histoNames.push(title);
    
    if(index == 0)
      histoXTitle = histoInfoJSON["fXaxis"]["fTitle"]
    arrayValues = [];
    for (i = 0; i < histoInfoJSON["fXaxis"]["fNbins"]; i++){
      // remember, we skip the first bin of data, since it is the underflow bin...
      var bin_center = (i*bin_width) + bin_width/2.0;
      
      if(index == 0){
        histoBinCenters.push(bin_center);
      }
      
      arrayValues.push(histoInfoJSON["fArray"][i+1]);

      csv_array += bin_center + "," +   histoInfoJSON["fArray"][i+1] + "\n";    
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
  //for(var ii = 0; ii < 5; ii++){
    csv_array_multi += histoBinCenters[ii];
    for(var jj = 0; jj < histoValues.length; jj++){
      csv_array_multi += ", " + histoValues[jj][ii];
    }
    csv_array_multi += "\n";    
  }

  
  //  g = new Dygraph(document.getElementById(divName),csv_array_multi,{title: 'multiple histograms'});
  if(deleteDygraph || gDygraphPointer[dygraphIndex] == 0){
    delete gDygraphPointer[dygraphIndex] ;
    gDygraphPointer[dygraphIndex]  = new Dygraph(document.getElementById(divName),csv_array_multi,
                                                 {title: 'multiple histograms', 'labelsSeparateLines' : true, 'legend' : 'always' });
  }else{
    gDygraphPointer[dygraphIndex].updateOptions( { 'file': csv_array_multi} );
  }
  document.getElementById("readstatus").innerHTML = "Rootana data correctly read";
  document.getElementById("readstatus").style.color = 'black';

}

