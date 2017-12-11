<!DOCTYPE html>
<html lang="en">
<head>
<?php
date_default_timezone_set('America/Los_Angeles');
$systemname = "SET THIS";
parse_str(str_replace("&amp;","&",$_SERVER['QUERY_STRING']), $pagequery);
if (isset($pagequery['m']))
	$m = $pagequery['m'];
else
	$m=date('Y')."-".date('n');
if (isset($pagequery['d']))
	$d = $pagequery['d'];
else
	$d=date('j');
if (isset($pagequery['tgs']))
	$tgs = $pagequery['tgs'];
else
	$tgs="ALL";

$dir = str_replace("-","/",$m)."/".$d."/";

echo "<base id=\"myBase\" href=\"".$dir."\">";

$talkgroups = array('ALL'=>"ALL", 100=>"Example1", 200=>"Example2", 1777=>"Bus Dispatch")
?>
<meta charset="UTF-8">
<title><?php echo $systemname." ";
if (isset($talkgroups[$tgs]) && ($tgs != "ALL")) echo $talkgroups[$tgs];
else echo "radio calls" ?> on <?php echo substr($m,5)."/".$d."/".substr($m,0,4); ?></title>
<script type="text/javascript">
	var player;
	var playlist;
	var currentcall;
	function init() {
		player = document.getElementById('audioplayer');
		playlist = document.getElementById('theplaylist');
		currentcall = playlist.getElementsByTagName('div')[0];
		if (currentcall.getElementsByTagName('a')[0])
			player.setAttribute('src',currentcall.getElementsByTagName('a')[0]);
                //document.getElementById("myBase").href = "<?php echo $dir; ?>";
		player.volume = 0.2;

		playlist.addEventListener('click',function (e) {
			//e.preventDefault();
		if (e.target.parentElement.getElementsByTagName('a')[0]) {
			currentcall.style.fontWeight = "normal";
			currentcall = e.target.parentElement;
			currentcall.style.fontWeight = "bold";
			player.setAttribute('src',currentcall.getElementsByTagName('a')[0]);
			player.load();
			player.play();
		} }, false);

		player.addEventListener('ended',function () { 
		if (currentcall.nextSibling) {
			currentcall.style.fontWeight = "normal";
			currentcall = currentcall.nextSibling;
			currentcall.style.fontWeight = "bold";
			player.setAttribute('src',currentcall.getElementsByTagName('a')[0]);
			player.load();
			player.play();
		}
		}, false);

	}
	window.onload=init;
</script>
<style type="text/css">span {padding-right: 10px; display: table-cell; max-width: 550px;}</style>
<body style="font-family: Arial;" ><div style="position:fixed; background: white; top: 0; width: 100%;">
<h1><?php echo $systemname." ";
if (isset($talkgroups[$tgs]) && ($tgs != "ALL")) echo $talkgroups[$tgs];
else echo "radio calls" ?> on <?php echo substr($m,5)."/".$d."/".substr($m,0,4); ?></h1>
<form>Change: Month: <select name="m">
<?php foreach (glob("2*/*", GLOB_ONLYDIR) as $mon) {
	echo '<option value="'.str_replace("/","-",$mon).'"';
	if ($mon == str_replace("-","/",$m)) echo ' selected="selected"';
	echo '>'.$mon.'</option>
'; }
unset ($mon);
echo '</select> Day: <select name="d">';
for ($x = 1; $x <= 31; $x++) {
	echo '<option value="'.$x.'"';
	if ($x == $d) echo ' selected="selected"';
	echo '>'.$x.'</option>
'; }
unset($x);
echo '</select> Channel(s): <select name="tgs">';

foreach ($talkgroups as $thistg => $thisvalue) {
	echo '<option value="'.$thistg.'"';
	if ($tgs == $thistg) echo ' selected="selected"';
	echo '>'.$thisvalue.'</option>
';
}
unset($thistg); ?>
</select> <input type="submit" value="Go"></form>

	<audio id="audioplayer" src="http://www.lalarm.com/en/images/silence.wav" preload="none" tabindex="0" controls>
		Sorry, your browser does not support HTML5 audio.
	</audio></div>

        <p style="font-weight: bold; margin-top: 150px;">Click on a row to begin sequential playback, click file size to download</p>

	<div id="theplaylist" style="display: table;">
<?php
if (file_exists($dir)) {
chdir($dir);
foreach (glob("*.wav") as $file) {
  if (filesize($file) > 10240) {
  $exploded = explode("-",str_replace("_","-",substr($file,0,-4)));
  if (($exploded[0] == $tgs) || ($tgs == "ALL")) {
    echo "<div style=\"display: table-row;\"><span>";
    echo date("H:i:s",$exploded[1])."</span><span>";
    if (isset($talkgroups[$exploded[0]])) echo $talkgroups[$exploded[0]]; else echo $exploded[0];
    echo "</span><span>";
    echo sprintf($exploded[2]/1000000)." MHz</span><span><a href=\"" . $file . "\">".round(filesize($file) / 1024)."k</a></span></div>
"; } } } }
else echo "Pick a different date or channel";
?>
</div></body></html>
