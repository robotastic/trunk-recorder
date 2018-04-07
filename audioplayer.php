<?php

$dateView = new DateTime(($_GET['dateView']) ?? null);
$filePath = $dateView->format('Y/n/j/');
$TGIDView = ($_GET['TGID']) ?? "*";

$TGList = [
    '*' => 'ALL',
    1767 => 'Example1',
    1777 => 'Example2'
];

?>
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Trunk Recorder Audio Player</title>
        <base id="filePath" href="<?=$filePath?>">
        <style>
            span {
                padding-right: 10px;
                display: table-cell;
                max-width: 550px;
            }
            body {
                font-family: Arial;
            }
            #playlist {
                display: table;
            }
            #playlist div {
                display: table-row;
            }
            #audioPlayer {
                position: fixed;
                background: white;
                top: 0;
                width: 100%;
            }
            body>p {
                font-weight: bold;
                margin-top: 150px;
            }
        </style>
    </head>
    <body>
        <div id="audioPlayer">
            <h1><?=(isset($TGList[$TGIDView]) AND ($TGIDView != "*")) ? $TGList[$TGIDView] : "Radio calls"?> on <?=$dateView->format('n/j/Y')?></h1>
            <form>
                Date:
                <input type="date" name="dateView" value="<?=$dateView->format('Y-m-d')?>">
                Channel(s):
                <select name="tgs">
<?php   foreach ($TGList as $TGID => $TGString): ?>
                    <option value="<?=$TGID?>"<?=($TGIDView == $TGID) ? ' selected="selected"' : ''?>><?=$TGString?></option>
<?php   endforeach; ?>
                </select>
                <input type="submit" value="Go">
            </form>
            <audio id="audio" preload="none" tabindex="0" controls>
                <source type="audio/wav" src="http://www.lalarm.com/en/images/silence.wav" />
                Sorry, your browser does not support HTML5 audio.
            </audio>
        </div>
        <p>Click on a row to begin sequential playback, click file size to download</p>
        <div id="playlist">
<?php   try {
            foreach (new DirectoryIterator('./' . $filePath) as $file):
                list($TGID, $timestamp, $frequency, $extension) = split('[_.-]', $file->getFilename());
                if (($TGID == $TGIDView) OR ($TGIDView == "*") AND $extension == 'wav'):    ?>
            <div>
                <span><?=date("H:i:s", $timestamp)?></span>
                <span><?=(isset($TGList[$TGID])) ? $TGList[$TGID] : $TGID?></span>
                <span><?=sprintf('%3.6f MHz', $frequency / 1000000)?></span>
                <span><a href="<?=$file->getFilename()?>"><?=round($file->getSize() / 1024)?>k</a></span>
            </div>
<?php           endif;  ?>
<?php       endforeach; ?>
<?php   } catch (UnexpectedValueException $e) {   ?>
            <p>Pick a different date</p>
<?php   }  ?>
        </div>
        <script>
            document.addEventListener('DOMContentLoaded', () => {
                var audio          = document.getElementById('audio');
                    playlist       = document.getElementById('playlist');
                    playlist_index = 0;

                audio.volume = 0.2;
                audio.addEventListener('ended', () => {
                    if (playlist_index != (playlist.getElementsByTagName('div').length - 1))
                    {
                        playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "normal";
                        playlist_index++;

                        var mp3link = playlist.getElementsByTagName('div')[playlist_index];
                        var mp3File =  mp3link.getElementsByTagName('a')[0];
                        var sources = document.getElementsByTagName('source')[0].setAttribute('src', mp3File);
                                      playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "bold";

                        audio.load();
                        audio.play();
                    }
                }, false);

                playlist.addEventListener('click', (e) => {
                    var mp3File = e.target.parentElement.getElementsByTagName('a')[0];
                    for (var i = 0; i < playlist.getElementsByTagName("a").length; i++)
                    {
                        if (playlist.getElementsByTagName("a")[i] == mp3File)
                        {
                            playlist.getElementsByTagName("div")[playlist_index].style.fontWeight = "normal";
                            playlist_index = i;
                            playlist.getElementsByTagName("div")[i].style.fontWeight = "bold";

                            var sources = document.getElementsByTagName('source');
                                sources[0].setAttribute('src', mp3File);

                            audio.load();
                            audio.play();
                        }
                    }
                }, false);
            }, false);
        </script>
    </body>
</html>
