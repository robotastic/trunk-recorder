<!DOCTYPE html>
<html lang="en">
<head>
<?php 
date_default_timezone_set('America/Los_Angeles');
$m=date('Y')."-".date('n'); $d=date('j'); $tgs="ALL";
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

$talkgroups = array('ALL'=>"ALL", 1767=>"Example1", 1777=>"Example2")
?>
<meta charset="UTF-8">
<title>Trunk Recorder audio player</title>
<script>
	var audio;
	var playlist;
	var debug;
	var playlist_index;
	function init() { 
		playlist_index = 0;
		audio = document.getElementById('audio');
		playlist = document.getElementById('playlist');
		debug = document.getElementById('debug');
                //document.getElementById("myBase").href = "<?php echo $dir; ?>";
		audio.volume = 0.2;

		playlist.addEventListener('click',function (e) {
			//e.preventDefault();
			var mp3File = e.target.parentElement.getElementsByTagName('a')[0];
			for (var i=0; i<playlist.getElementsByTagName("a").length; i++) {
			if (playlist.getElementsByTagName("a")[i] == mp3File) {
				playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "normal";
				playlist_index = i; playlist.getElementsByTagName("div")[i].style.fontWeight = "bold";
				var sources = document.getElementsByTagName('source');
				sources[0].setAttribute('src',mp3File);
				audio.load();
				audio.play();
			} }
		}, false);

		audio.addEventListener('ended',function () { 
		if(playlist_index != (playlist.getElementsByTagName('div').length - 1)){
			playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "normal";
			playlist_index++;
			var mp3link = playlist.getElementsByTagName('div')[playlist_index];
			var mp3File = mp3link.getElementsByTagName('a')[0];
			var sources = document.getElementsByTagName('source');
			sources[0].setAttribute('src',mp3File);
			playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "bold";
			audio.load();
			audio.play();
		}
		}, false);
	
	}
	window.onload=init;
</script><style>span {padding-right: 10px; display: table-cell; max-width: 550px;}</style>
<body style="font-family: Arial;" ><div style="position:fixed; background: white; top: 0; width: 100%;">
<h1><?php if (isset($talkgroups[$tgs])) echo $talkgroups[$tgs]; 
	else echo "Radio calls" ?>Calls on <?php echo substr($m,5)."/".$d."/".substr($m,0,4); ?></h1>
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

	<audio id="audio" preload="none" tabindex="0" controls>
		<source type="audio/wav" src="http://www.lalarm.com/en/images/silence.wav" />
		Sorry, your browser does not support HTML5 audio.
	</audio></div>

        <p style="font-weight: bold; margin-top: 150px;">Click on a row to begin sequential playback, click file size to download</p>

	<div id="playlist" style="display: table;">
<?php
if (file_exists($dir)) {
chdir($dir);
foreach (glob("*.wav") as $file) {
  $exploded = explode("-",str_replace("_","-",substr($file,0,-4)));
  if (($exploded[0] == $tgs) || ($tgs == "ALL")) {
    echo "<div style=\"display: table-row;\"><span>";
    echo date("H:i:s",$exploded[1])."</span><span>";
    if (isset($talkgroups[$exploded[0]])) echo $talkgroups[$exploded[0]]; else echo $exploded[0];
    echo "</span><span>";
    echo sprintf($exploded[2]/1000000)."</span><span><a href=\"" . $file . "\">".round(filesize($file) / 1024)."k</a></span></div>
"; } } }
else echo "Pick a different date";
?>
	</div>

</body></html>
