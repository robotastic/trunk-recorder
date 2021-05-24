<?php  //created by Jason McHuff, http://www.rosecitytransit.org/

$pagetitle = "trunk-recorder audio player";
$filetype = "m4a";

$default_system = ""; //shortName of system to show by default, from $shortNames below or config.json
$default_talkgoup = 0; //default TG to show, see $talkgroups below

//SET THIS (your trunk-recorder config.json file):
$config_json = "../../trunk-build/config.json";

//OR THESE: (to use, make sure the above is not valid)
$captureDir = ""; //where the audio files are saved, from trunk-recorder's config.json
				//if this file is in the captureDir (next to directories matching shortNames, this can be left blank
				//can be relative to this file or start w/ "/" for absolute path
//leave it and $default_system blank if these files are in captureDir next to the year directories (not using multi-system mode)

//List of talkgroups to display in channel menu, delete to list all in talkgroups file, format is $shortName-$talkgroup
//if you only have 1 system, can omit shortName: array(0=>"ALL", 100=>"Example 1");
$talkgroups = array('sysA-0'=>"ALL system 1", 'sysA-100'=>"Example 1", 'sysB-200'=>"Example 2", 'Group1'=>"Group 1");
//Groups of talkgroups to display in channel menu, put in $talkgroups
$groups = array('Group1'=>array('sysA-100', 'sysB-200'));



if ((strlen($captureDir) > 1) && (substr($captureDir, -1) != "/"))
  $captureDir .= "/";

/* if (strpos($shortNames, ",") !== false )
  $shortNames = explode(",", $shortNames);
else $shortNames = array(0=>$shortNames); */

if (is_file($config_json)) {
  $string = file_get_contents($config_json);
  $json_a = json_decode($string, true);

  $dir = "";
  if (strrpos($config_json, "/") !== false)
    $dir = substr($config_json, 0, strrpos($config_json, "/"));

  foreach($json_a["systems"] as $sys) {
    $talkgroups[$sys["shortName"]."-0"] = "ALL ".$sys["shortName"];
    if (is_file($dir."/".$sys["talkgroupsFile"])) {
      $talkgroupsfile = file($dir."/".$sys["talkgroupsFile"]);
      foreach ($talkgroupsfile as $filerow) {
        $filerow = explode(",", $filerow);
        $talkgroups[$sys["shortName"]."-".$filerow[0]] = $filerow[4];
      }
    }
  }
  unset($sys,$talkgroupsfile,$dir,$filerow);
}
?>
