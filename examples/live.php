<?php  //created by Jason McHuff, http://www.rosecitytransit.org/
include("live.config.php");
?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title><?php echo $pagetitle; ?></title>
<script type="text/javascript">
<?php date_default_timezone_set('America/Los_Angeles');
if (isset($_SERVER['QUERY_STRING']) && (strlen($_SERVER['QUERY_STRING']) < 35))
  $calls = explode("+",$_SERVER['QUERY_STRING']);
if (end($calls) == "fewer") {
	$fewer = array_pop($calls);
	echo '	var fewer = "+fewer";
';
} else {
	$fewer = false;
	echo '	var fewer = "";
';
}
if (!isset($calls[0]))
	$calls[0] = $default_system;
if (!isset($calls[3]))
	$calls[3] = date('Y-n-j');
if (!isset($calls[1]) || ($calls[1] == "")) {
	$calls[1] = "ALL";
	echo '	var calls="ALL";
';
} else echo '	var calls = "'.$_SERVER['QUERY_STRING'].'";
'; ?>
	var player;
	var playlist;
	var currentcall;
	var channel;
	var e;
	var loadcalls;
	function init() {
		e = document.createElement('div');
		player = document.getElementById('audioplayer');
		playlist = document.getElementById('theplaylist');
		channel = document.getElementById('channels');
		getcalls();
		player.volume = 0.2;

		playlist.addEventListener('click',function (e) {
			//e.preventDefault();
			if ((e.target.parentElement.nodeName == "DIV") && (e.target.parentElement != playlist) && e.target.parentElement.getElementsByTagName('a')[1]) {
				playnext(e.target.parentElement);
				e.target.parentElement.style.backgroundColor = "#FFFF99";
			}
			else if ((e.target.parentElement == playlist) && e.target.getElementsByTagName('a')[1]) {
				playnext(e.target);
				e.target.style.backgroundColor = "#FFFF99";
			}
		}, false);

		player.addEventListener('ended',function () {
			if (currentcall.nextSibling && (document.getElementById('autoplay').checked == true)) {
				playnext(currentcall.nextSibling);
			}
		}, false);
	}

	window.onload=init;
	<?php if (!isset($calls[2]))
		echo "	var loadcalls=setInterval(getcalls, 20000);"; ?>

	function getcalls() {
		if ((document.getElementById('addnew').checked == true) || (playlist.innerHTML == ''))
		downloadUrl("calls.php?"+calls, function(data) {
			newcalls = document.getElementById('newcalls');
			if (newcalls)
				newcalls.removeAttribute("id");
			calls += "+";
			calls = calls.substr(0, calls.indexOf("+"))+"+"+data.match(/\d+/)+fewer;
			data = data.replace(/\d+/,"");
			e.innerHTML = data;
			while(e.firstChild) {
				playlist.appendChild(e.firstChild);
			}
			if (!currentcall && playlist.childNodes) {
				playnext(playlist.firstChild);
			}
			if ((document.getElementById('autoplay').checked == true) && (player.ended == true) && newcalls) {
				playnext(newcalls);
				window.location="#newcalls";
			}
		});
	}

	function playnext(nextcall) {
		do {
			if (nextcall.getElementsByTagName('a')[1]) {
				if (currentcall)
				currentcall.style.fontWeight = "normal";
				nextcall.style.fontWeight = "bold";
				nextcall.style.backgroundColor = "#FFFFCC";
				player.setAttribute('src',nextcall.getElementsByTagName('a')[1]);
				player.load();
				if (currentcall)
				player.play();
				currentcall = nextcall;
				break;
			}
		} while (nextcall = nextcall.nextSibling)
	}

	function changecalls(day,today) {
		newtitle = "<?php echo $pagetitle; ?>"+channel.options[channel.selectedIndex].text;
		calls = channel.value;
		if (day)
			calls += "+00";
		else
			loadcalls=setInterval(getcalls, 20000);
		if (!today) {
			calls += "+"+document.getElementById("m").value+"-"+document.getElementById("d").value;
			newtitle += " on "+document.getElementById("m").value+"-"+document.getElementById("d").value;
			clearInterval(loadcalls);
		}
		if (document.getElementById('fewer').checked == true) {
			fewer = "+fewer";
			calls += "+fewer";
		}
		else
			fewer = "";
		document.title = newtitle;
		document.getElementById('curchan').innerHTML = "<a href=\"?"+calls+"\">"+newtitle+"</a>";
		currentcall = undefined;
		playlist.innerHTML="";
		getcalls();
	}

function createXmlHttpRequest() {
 try {
   if (typeof ActiveXObject != 'undefined') {
     return new ActiveXObject('Microsoft.XMLHTTP');
   } else if (window["XMLHttpRequest"]) {
     return new XMLHttpRequest();
   }
 } catch (e) {
   changeStatus(e);
 }
 return null;
};

function downloadUrl(url, callback) {
 var status = -1;
 var request = createXmlHttpRequest();
 if (!request) {
   return false;
 }

 request.onreadystatechange = function() {
   if (request.readyState == 4) {
     try {
       status = request.status;
     } catch (e) {
       // Usually indicates request timed out in FF.
     }
     if (status == 200) {
       callback(request.responseText, request.status);
       request.onreadystatechange = function() {};
     }
   }
 }
 request.open('GET', url, true);
 try {
   request.send(null);
 } catch (e) {
   changeStatus(e);
 }
};
</script>
<style>#theplaylist span { padding-left: 5px; padding-right: 5px; padding-top: 1px; padding-bottom: 1px; display: table-cell; max-width: 450px; }
#theplaylist div {display: table-row; cursor: default;} .e { color: red;}
#theplaylist a { text-decoration: none; color: black; }
#theplaylist { display: table; margin-left: auto; margin-right: auto; }
#curchan { margin-top: 0; margin-bottom: 0; font-size: large; }
#infop { font-weight: bold; margin-top: 100px; text-align: center; }
#player { position:fixed; background: white; top: 0; width: 100%;  background: #FFFFEE; text-align: center; }</style>



<body style="font-family: Arial;" ><div id="player">
<h1 id="curchan"><?php echo $pagetitle; ?></h1>
<form action="" name="changecall">Channel(s): <select name="channels" id="channels">

foreach ($tglist as $thistg => $thisvalue) {
	echo '<option value="'.$thistg.'"';
	if ($calls[1] == $thistg) echo ' selected="selected"';
	echo '>'.$thisvalue.'</option>
';
}
unset($thistg); ?>
</select><input type="button" value="Now" onclick="changecalls(false, true);"><input type="button" value="Today" onclick="changecalls(true, true);">
<select name="m" id="m">
<?php $themon = explode("-",$calls[3]);
foreach (glob("2*/*", GLOB_ONLYDIR) as $mon) {
	echo '<option value="'.str_replace("/","-",$mon).'"';
	if ($mon == $themon[0]."/".$themon[1]) echo ' selected="selected"';
	echo '>'.$mon.'</option>
'; }
unset ($mon);
echo '</select>/<select name="d" id="d">';
for ($x = 1; $x <= 31; $x++) {
	echo '<option value="'.$x.'"';
	if ($x == $themon[2]) echo ' selected="selected"';
	echo '>'.$x.'</option>
'; }
unset($x); ?></select><input type="button" value="Day" onclick="changecalls(true, false);"> <label><input id="fewer"<?php
if ($fewer) echo ' checked="checked"' ?> type="checkbox"/>Fewer columns</label>
	<br /><audio id="audioplayer" src="blank.mp3" preload="none" tabindex="0" controls>
		Sorry, your browser does not support HTML5 audio.
	</audio> <label><input id="addnew" type="checkbox" checked="checked" />Add new calls</label> <label><input id="autoplay" type="checkbox" />Auto play</label></div>

	<p id ="infop">Click on a row to play, click # for link, or click file size to download</p>

	<div id="theplaylist"></div>

</body></html>
