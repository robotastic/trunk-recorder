<?php //created by Jason McHuff, http://www.rosecitytransit.org/
//style from: Topnew CMS Chart v5.5.5 - Released on 2016.05.05
// The MIT License (MIT) Copyright (c) 1998-2015, Topnew Geo, http://topnew.net/chart

//Can specifiy log files to get data from in URL, e.g. decodechart.php?05-*-2021
//however each data point always takes up 1 px width (no adjustment based on amount)

//SET THESE:
//default amount of data points to include if no files specified
//I set controlWarnUpdate/controlWarnRate in config.json so rates always output once/minute  
$datapoints = 1440;

$chartwidth = 1500;

//shown at the top of the chart if no files specified
$charttitle = "Last 24 hours";

//can be used to identify which color is which system; can copy tspan if more than 1
$chartdesc = '(<tspan style="fill: red;">System 1</tspan>)';

$logsdir = "path/to/trunk-recorder/logs/";

//set "sys1" to the shortName in config.json; can copy the line if more than 1 system
$css = "<defs><style type=\"text/css\"><![CDATA[
#sys1{stroke:red;}



svg.chart{display:block}
.chart-tick{stroke:#ddd;stroke-width:1;stroke-dasharray:5,5}
.chart-bg{fill:#eee;opacity:.5}
.chart-box{fill:#fff;opacity:1}
text{font-family:Helvetica,Arial,Verdana,sans-serif;font-size:12px;fill:#666}
.line{fill:none;stroke-width:3}
.axisX{text-anchor:start}
.axisY{text-anchor:end}
]]></style></defs>";

header('Content-type: image/svg+xml');
header('Vary: Accept-Encoding');

$decode = array();
//parser for "Control Channel Message Decode Rate" lines from log files; creates array of $decode[date/time][system][rate]
function parsefile($filename) {
  global $decode;
  preg_match_all("/\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}).*\[(.*)\].*Control Channel Message Decode Rate: (.*)\/sec/", file_get_contents($filename), $logfile);
  foreach ($logfile[1] as $theentry => $datetime)
    $decode[$datetime][$logfile[2][$theentry]] = $logfile[3][$theentry];
    //if (isset($decode[1][$theentry-1]) && ($datetime == $decode[1][$theentry-1]))
  unset($theentry, $datetime);
}


//get the file names from URL
if (isset($_SERVER['QUERY_STRING']) && (strlen($_SERVER['QUERY_STRING']) > 4)) {
  foreach (glob($logsdir.$_SERVER['QUERY_STRING']."*.log") as $filename)
    parsefile($filename);
  $charttitle = $_SERVER['QUERY_STRING'];
}

//if no/little data, default to last 24 hours from yesterday's+today's logs
if (count($decode) < 5) {
  foreach (glob($logsdir.date("m-d-Y", mktime(0, 0, 0, date("m"), date("d")-1, date("Y")))."*.log") as $filename)
    parsefile($filename);
  foreach (glob($logsdir.date("m-d-Y")."*.log") as $filename)
    parsefile($filename);
  $decode = array_slice($decode, -1440);
}


//start the SVG
echo '<svg width="'.$chartwidth.'" height="450" viewBox="0 0 '.$chartwidth.' 450" class="chart" version="1.0" xmlns="http://www.w3.org/2000/svg">
'.$css.'
<rect width="'.$chartwidth.'" height="450" class="chart-bg" />
<rect x="25" y="25" width="'.($chartwidth-50).'" height="400" class="chart-box" />
';

//draw the y axis
for($i = 0; $i <= 9; $i++)
  echo '<text class="axisY" x="25" y="'.(($i*50)+25).'">'.(40-($i*5)).'</text>
<line class="chart-tick" x1="25" x2="'.($chartwidth-25).'" y1="'.(($i*50)+25).'" y2="'.(($i*50)+25).'" />
';
echo '<text y="15" x="750" text-anchor="middle">'.$charttitle.' control channel decoding rates 
'.$chartdesc.'</text>';

//var_dump($decode);

//loop through the data, using x-value as "row"
$currenthour = substr(key($decode),0,13);
$datarow = 25;
$datalines = array();
foreach ($decode as $datetime => $entries) {
  //label the axis every new hour
  if ($currenthour != substr($datetime,0,13)) {
    echo '
<line class="chart-tick" x1="'.$datarow.'" x2="'.$datarow.'" y1="25" y2="425" />
<text class="axisX" transform="rotate(-90)" y="'.$datarow.'" x="-425">'.$datetime.'</text>';
    $currenthour = substr($datetime,0,13);
  }
  //create the data lines (ignote how $datalines[$system] isn't set the first time)
  foreach ($entries as $system => $datavalue)
    @$datalines[$system] .= $datarow.",".(((40 - $datavalue)*10)+25)." ";
  $datarow++;
}

//draw the data lines
foreach ($datalines as  $system => $dataline)
  echo '
<polyline id="'.$system.'" class="line" points="'.$dataline.'" />';

echo '</svg>';
?>