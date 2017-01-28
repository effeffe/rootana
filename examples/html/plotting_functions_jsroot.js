




var mdi = null, cnt = 0, drawopt = null, addr = null;

function initialize(){

  JSROOT.RegisterForResize('graphdiv');
  JSROOT.RegisterForResize('graphdiv21');
  JSROOT.RegisterForResize('graphdiv22');
  
}


function xhrErrorResponse(listDirectories, errorString){

  console.log("Error ! " + errorString);
  document.getElementById("readstatus").innerHTML = "Couldn't get histogram data; request = "+ listDirectories + "; error = " + errorString + ". Did rootana httpserver die?";
  document.getElementById("readstatus").style.color = 'red';    
  // If we couldn't find histogram, try forcing re-find of rootana directory
  gFoundRootanaDir = false;
  xhrAlreadyInFligthJSROOT = false;
  find_active_root_directory();

}

var xhrAlreadyInFligthJSROOT = false;


// This method will create a plot in the requested 
// divs using the requested histograms.
// Use JSROOT plotting of data
function plotAllHistogramsJSROOT(plotType,divNames, histogramNameList, deleteDygraph){

  // Find the directory structure, if it is not yet found.
  if(!gFoundRootanaDir){
    find_active_root_directory();
    return;
  }

  // Don't make another request if last request isn't finished.
  if(xhrAlreadyInFligthJSROOT) return;
  xhrAlreadyInFligthJSROOT = true;

  // Wrap the request in promise; will combine multiple items (if there are more than 1) into single XHR request
  var listDirectories = "";
  for(var index = 0; index < histogramNameList.length; index++){
    var name = active_directory + "/" + histogramNameList[index];
    listDirectories += name + "/root.json\n";
  }    

  // compose URL
  var url = rootana_dir + "multi.json?number="+String(histogramNameList.length);
 
  getUrl(url, listDirectories,divNames).then(function(response) {
    
    xhrAlreadyInFligthJSROOT = false;
    
    var histo = JSON.parse(response);    
    // Little hack from Sergey to distinguish the different responses from THttpServer

    if(plotType == "overlay"){
      $("#graphdiv").html("");
    }
    for (var i=0;i<histo.length;++i){
      histo[i] = JSROOT.JSONR_unref(histo[i]);
      if(plotType == "overlay"){         
        //JSROOT.clear(divNames[0]);
        if(i==0){
          JSROOT.draw(divNames[0], histo[i], "");
        } else {
          JSROOT.draw(divNames[0], histo[i], "SAME");
        }
      }else{
        JSROOT.redraw(divNames[i], histo[i], "colz");
      }
    }


     document.getElementById("readstatus").innerHTML = "Rootana data correctly read";
     document.getElementById("readstatus").style.color = 'black';      

   }).catch(function(error) {  // Handle exception if we didn't find the histogram...

     promiseAlreadyInFligth = false;
     xhrErrorResponse(listDirectories,error);
    
   });
  


}
