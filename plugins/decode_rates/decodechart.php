<?php //created by Jason McHuff, http://www.rosecitytransit.org/

//style from: Topnew CMS Chart v5.5.5 - Released on 2016.05.05
// The MIT License (MIT) Copyright (c) 1998-2015, Topnew Geo, http://topnew.net/chart

//Can specifiy how many points to load using the 'amount' URL parameter
//as well as start timestamp using the 'since' parameter

//SET THESE:
//default amount of data points to include if no files specified
$datapoints = 1440;

$chartwidth = 1500;

//shown at the top of the chart if no files specified
$charttitle = "Control channel decoding rates";

//can be used to identify which color is which system; can copy tspan if more than 1
$chartdesc = '(<tspan style="fill: red;">System 1</tspan>)';

$captureDir = "path/to/captureDir/";

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

//get the since and amount paramaters if set in the URL
if (isset($_SERVER['QUERY_STRING'])) {
  parse_str($queryString, $queryArray);
  if (!$queryArray["since"] && $queryArray["amount"]) {
    $charttitle .= " last ".$queryArray["amount"]." points";
    $decode = array_slice(file($captureDir."decode_rates.csv", FILE_IGNORE_NEW_LINES), 0-$datapoints);
  } elseif ($queryArray["since"]) {
    $charttitle .=  " since ".date("r");
    $wholefile = file($captureDir."decode_rates.csv", FILE_IGNORE_NEW_LINES);
    foreach ($wholefile as $fileline)
      if (substr($fileline,0,strpos($fileline,",")) >= $queryArray["since"])
        $decode[] = $fileline;
    if ($queryArray["amount"]) {
      $charttitle .= ", ".$queryArray["amount"]." points";
      $decode = array_slice($decode, 0, $queryArray["amount"]);
    }
  }
}

if (count($decode) < 5) {
  $decode = array_slice(file($captureDir."decode_rates.csv", FILE_IGNORE_NEW_LINES), 0-$datapoints);
}


//start the SVG
echo '<svg width="'.(count($decode)+60).'" height="450" viewBox="0 0 '.(count($decode)+60).' 450" class="chart" version="1.0" xmlns="http://www.w3.org/2000/svg">
'.$css.'
<rect width="'.(count($decode)+60).'" height="450" class="chart-bg" />
<rect x="25" y="25" width="'.(count($decode)+10).'" height="400" class="chart-box" />
';

//draw the y axis
for($i = 0; $i <= 9; $i++)
  echo '<text class="axisY" x="25" y="'.(($i*50)+25).'">'.(40-($i*5)).'</text>
<line class="chart-tick" x1="25" x2="'.(count($decode)+35).'" y1="'.(($i*50)+25).'" y2="'.(($i*50)+25).'" />
';
echo '<text y="15" x="750" text-anchor="middle">'.$charttitle.' '.$chartdesc.'</text>';

//loop through the data, using x-value as "row"
$currenthour = NULL;
$datarow = 25;
$datalines = array();
foreach ($decode as $decodeline) {
  $decodeline = explode(",", $decodeline);
  //label the axis every new hour
  if ($currenthour != date('YmdH', $decodeline[0])) {
    echo '
<line class="chart-tick" x1="'.$datarow.'" x2="'.$datarow.'" y1="25" y2="425" />
<text class="axisX" transform="rotate(-90)" y="'.$datarow.'" x="-425">'.date('Y-m-d H:m:s', $decodeline[0]).'</text>';
    $currenthour = date('YmdH', $decodeline[0]);
  }
  unset($decodeline[0]);
  //create the data lines (ignote how $datalines[$system] isn't set the first time)
  foreach ($decodeline as $system => $datavalue)
    @$datalines[$system] .= $datarow.",".(((40 - $datavalue)*10)+25)." ";
  $datarow++;
}

//draw the data lines
foreach ($datalines as  $system => $dataline)
  echo '
<polyline id="system'.$system.'" class="line" points="'.$dataline.'" />';

echo '</svg>';
?>
