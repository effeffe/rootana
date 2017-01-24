




var mdi = null, cnt = 0, drawopt = null, addr = null;

function initialize(){

  JSROOT.RegisterForResize('graphdiv');
  JSROOT.RegisterForResize('graphdiv21');
  JSROOT.RegisterForResize('graphdiv22');
  
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
  
  for(var index = 0; index < histogramNameList.length; index++){

    var url = rootana_dir + active_directory + "/" + histogramNameList[index] + "/root.json.gz?compact=3";
    console.log("send this request: " + url);


    var req = JSROOT.NewHttpRequest(url, 'object', function(histo) {
      if (!histo) {
        console.log("Fail to get histo");
        document.getElementById("readstatus").innerHTML = "Couldn't get histogram data; request = "+ listDirectories + "; error = " + error + ". Did rootana httpserver die?";
        document.getElementById("readstatus").style.color = 'red';    
        // If we couldn't find histogram, try forcing re-find of rootana directory
        gFoundRootanaDir = false;
        find_active_root_directory();
        return;
      }
      
      // redraw histogram at specified frame
      JSROOT.redraw(this.frame, histo, "colz");
      //JSROOT.redraw(frame, histo, "hist");

      xhrAlreadyInFligthJSROOT = false;
      document.getElementById("readstatus").innerHTML = "Rootana data correctly read";
      document.getElementById("readstatus").style.color = 'black';      

    });

    
    req.frame = divNames[index];
    req.send(null);

  }

  return;  

}
