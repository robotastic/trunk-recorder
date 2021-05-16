<?php  //created by Jason McHuff, http://www.rosecitytransit.org/

$pagetitle = "trunk-recorder audio player";
$filetype = "m4a";

//SET THIS (your trunk-recorder config.json file; can be in a different directory):

$config_json = "examples/config-multi-system.json";

//OR THESE: (to use, make sure the above is not valid)

$shortNames = array("sys1", "sys2"); //the one(s) specified in trunk-recorder's config.json
$default_system = "sys1"; //which one should show on default
$captureDir = ""; //where the audio files are saved, from trunk-recorder's config.json
				//if this file is in the captureDir (next to directories matching shortNames, this can be left blank
				//can be relative to this file or start w/ "/" for absolute path
//make all 3 variables blank if this file is in captureDir next to the year directories (not using multi-system mode)

$talkgroups["sys1"] = array('ALL'=>"ALL", 100=>"Example 1", 200=>"Example 2", 'Group1'=>"Group 1");
$groups = array('Group1'=>array(100,200));

if ((strlen($captureDir) > 1) && (substr($captureDir, -1) != "/"))
  $captureDir .= "/";

if (is_file($config_json)) {
  $string = file_get_contents($config_json);
  $json_a = json_decode($string, true);

  $dir = "";
  if (strrpos($config_json, "/") !== false)
    $dir = substr($config_json, 0, strrpos($config_json, "/"));

  foreach($json_a["systems"] as $sys) {
    if (is_file($dir.$sys["talkgoupsFile"])) //check this
      $systems[$sys["shortName"]] = $sys["talkgoupsFile"];
  }
  $default_system = $json_a["systems"][0]["shortName"];
}
?>
