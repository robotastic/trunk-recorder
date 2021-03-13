<?php chdir('/var/www/html/radio/cc/2018/2/28/');
date_default_timezone_set('America/Los_Angeles');
foreach (glob("*_*.mp3") as $file) {
$exploded = explode("-",substr($file,0,strpos($file,"_")));
$newname = date('His', $exploded[1])."-".$exploded[0];

if (file_get_contents(substr($file,0,-4).".json")) {
$string = file_get_contents(substr($file,0,-4).".json");
$json_a = json_decode($string, true);
$srcList=$json_a['srcList'];
//$srcList=array_unique($srcList);

foreach ($srcList as $source)
$newname.= "-".$source['src']; }

$newname.= "-cc.mp3";
//$tg = $exploded[0];
//$exploded[0] = "";
//$exploded = array_unique($exploded);
//$exploded[0] = $tg;
rename ($file, $newname);
}
?>
