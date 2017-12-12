<!DOCTYPE html>
<html lang="en">
<head>
<?php

date_default_timezone_set('America/Los_Angeles');
$systemname = "SET THIS";
$chanlist = "/path/to/trunk-recorder/ChanList.csv";
$talkgroups = array('ALL'=>"ALL", 100=>"Example1", 200=>"Example2", 'Group1'=>"Group 1");
$Group1 = array(100,200); //optional groups of talkgroups
//may be better to fill $talkgroups instead of $chanlist so ChanList.csv
//doesn't have to be read and parsed every time the Web page is loaded
//if using ChanList.csv, make sure it is readable by e.g. apache
$desc=4; //set to 3 to display Alpha Tags instead of Descriptions

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


if (isset($chanlist) && file_exists($chanlist)) {
	$talkgroups = array('ALL'=>"ALL");
	$filedata = file($chanlist,FILE_IGNORE_NEW_LINES);
	foreach ($filedata as $wholeline) {
		$allparts = explode(",", $wholeline);
		$talkgroups[$allparts[0]] = $allparts[$desc];
	}
}
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
	playlist = document.getElementById('theplaylist');
	if (playlist.getElementsByTagName('div')[0]) {
		player = document.getElementById('audioplayer');
		currentcall = playlist.getElementsByTagName('div')[0];
		currentcall.style.fontWeight = "bold";
		player.setAttribute('src',currentcall.getElementsByTagName('a')[0]);
		player.volume = 0.2;

		playlist.addEventListener('click',function (e) {
			//e.preventDefault();
			if ((e.target.parentElement.nodeName == "DIV") && (e.target.parentElement != playlist) && e.target.parentElement.getElementsByTagName('a')[0]) {
				playnext(e.target.parentElement);
			}
			else if ((e.target.parentElement == playlist) && e.target.getElementsByTagName('a')[0]) {
				playnext(e.target);
			}
		}, false);

		player.addEventListener('ended',function () {
			if (currentcall.nextSibling && (document.getElementById('continuous').checked == true)) {
				playnext(currentcall.nextSibling);
			}
		}, false);

	} }

	function playnext(nextcall) {
		currentcall.style.fontWeight = "normal";
		currentcall = nextcall;
		currentcall.style.fontWeight = "bold";
		player.setAttribute('src',currentcall.getElementsByTagName('a')[0]);
		player.load();
		player.play();
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
	</audio><label><input id="continuous" type="checkbox" checked="checked" />Play continuously</label></div>

        <p style="font-weight: bold; margin-top: 150px;">Click on a row to begin sequential playback, click file size to download</p>

	<div id="theplaylist" style="display: table;"><?php
if (file_exists($dir)) {
	chdir($dir);
	if (isset($$tgs) && is_array($$tgs))
		$tgs = $$tgs;
	else $tgs = explode(",",$tgs);
	foreach (glob("*.wav") as $file) {
		if (filesize($file) > 10240) {
			$exploded = explode("-",str_replace("_","-",substr($file,0,-4)));
			if (in_array($exploded[0], $tgs) || ($tgs[0] == "ALL")) {
				echo "<div 
style=\"display: table-row;\"><span>";
				echo date("H:i:s",$exploded[1])."</span><span>";
				if (isset($talkgroups[$exploded[0]]))
					echo $talkgroups[$exploded[0]]; 
				else	echo $exploded[0];
				echo "</span><span>".sprintf($exploded[2]/1000000)." MHz</span><span><a href=\"" . $file . "\">".round(filesize($file) / 1024)."k</a></span></div>"; 
			}
		} 
	}
}
else echo "Pick a different date";
?>
</div></body></html>
