<?php //created by Jason McHuff, http://www.rosecitytransit.org/
//SET THIS:
$captureDir = "/trunk-recorder/captureDir";

function processdir($dir) {
	chdir($dir);
	$output = "";
	foreach (glob("*.json") as $file) {
		$string = file_get_contents($file);
		$json_a = json_decode($string, true);
		$output .= $json_a['start_time'].",".($json_a['stop_time']-$json_a['start_time']).",0,".$json_a['talkgroup'].",".$json_a['emergency'].",0,0,0,";
		$sources = "";
		foreach ($json_a['srcList'] as $source) {
			$sources .= $source['src']."|";
		}
		if ($sources)
			$output .= substr($sources,0,-1);
		unset($source);
		foreach ($json_a['freqList'] as $freqs) {
			$output .= ",".(int)$freqs['freq']."|".(int)$freqs['len']."|".(int)$freqs['error_count']."|".(int)$freqs['spike_count'];
		}
		unset($freqs);
		$output .= PHP_EOL;
	}
	file_put_contents("calllog.txt", $output);
	unset($file, $output);
	foreach (glob("*", GLOB_ONLYDIR) as $innerdir)
		processdir($innerdir);
}

processdir($captureDir);
?>
