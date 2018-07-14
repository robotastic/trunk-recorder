<?php 
$TGS = [
    1767 => 'Example1',
    1777 => 'Example2'
];

$MHz = function (float $MHz): string {
    switch (TRUE)
    {
        case $MHz >= 136 AND $MHz <= 174:
            return 'bg-primary';
        case $MHz >= 380 AND $MHz <= 520:
            return 'bg-danger';
        case $MHz >= 764 AND $MHz <= 870:
            return 'bg-warning';
        case $MHz >= 896 AND $MHz <= 901:
        case $MHz >= 935 AND $MHz <= 940:
            return 'bg-success';
        default:
            return 'null';
    }
};

$files     = [];
$date      = (isset($_GET['date'])) ? new DateTimeImmutable($_GET['date']) : new DateTimeImmutable();
$TG        = (empty($_GET['TG'])) ? NULL : $_GET['TG'];

try  {
    $path = new DirectoryIterator('./audio/' . $date->format('Y/n/j') . '/');

    if (!$path->isDir())
        throw new Exception('No directory found for that date.');

    foreach ($path as $file)
    {
        $EXT = '.' . $file->getExtension();
        if (in_array($EXT, ['.m4a', '.wav']))
            continue;

        $Basename = $file->getBasename($EXT);
        # Probably a Me Bug ...
        if (strlen($Basename) < 20)
            continue;

        [$TGID, $TIME, $FREQ] = preg_split('/[-_]/', $Basename);
        substr($FREQ, 0, -4);
        if ($TGID != $TG AND $TG !== NULL)
            continue;

        $files[$file->getMTime()] = [$file->getFilename(), $file->getSize(), $TGID, $TIME, $FREQ];
    }

    ksort($files);

} catch (Exception $e) {
    $error = $e->getMessage();
}


?>
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <title>Trunk Player</title>
        <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.2/css/bootstrap.min.css" integrity="sha384-Smlep5jCw/wG7hdkwQ/Z5nLIefveQRIY9nfy6xoR1uRYBtpZgI6339F5dgvm/e9B" crossorigin="anonymous">
    </head>
    <body>
        <div class="container">
            <script>
                window.onload = function () { 
                    var index = 0,
                      sources = document.getElementsByTagName('source')[0],
                        audio = document.getElementsByTagName('audio')[0],
                         list = document.getElementsByTagName('tbody')[0];

                    list.addEventListener('click', function (e) {
                        var mp3File = e.target.parentElement.getElementsByTagName('a')[0];
                        for (var item = 0; item < list.getElementsByTagName("a").length; item++) {
                            if (list.getElementsByTagName("a")[item] == mp3File) {
                                list.getElementsByTagName("tr")[index].classList.toggle("text-attention");
                                index = item;   // Update the index to the item we clicked on.
                                list.getElementsByTagName("tr")[index].classList.toggle("text-attention");
                                sources.setAttribute('src', mp3File);
                                audio.load();
                                audio.play();
                            }
                        }
                    }, false);

                    audio.addEventListener('ended',function () {
                        if (index == (list.getElementsByTagName('tr').length - 1))
                            return;

                        list.getElementsByTagName("tr")[index].classList.toggle("text-attention");
                        index++;
                        var mp3File = list.getElementsByTagName('tr')[index].getElementsByTagName('a')[0];
                        sources.setAttribute('src', mp3File);
                        list.getElementsByTagName("tr")[index].classList.toggle("text-attention");
                        audio.load();
                        audio.play();
                    }, false);
                }
            </script>
            <style>
                #interface {
                    width: 100%;
                }
            </style>
            <div id="interface">
                <h1><?=(isset($TGS[$TG]) AND ($TG !== NULL)) ? $TGS[$TG] : "All Calls"?> on <?=$date->format('Y-m-d')?></h1>
                <form method="get">
                    <div class="row">
                        <div class="form-group col-lg-4">
                            <label class="form-control-label" for="date">Date</label>
                            <input class="form-control" id="date" name="date" type="date" value="<?=$date->format('Y-m-d')?>" />
                        </div>
                        <div class="form-group col-lg-4">
                            <label class="form-control-label" for="TG">Talk Group</label>
                            <select class="form-control" id="TG" name="TG">
                                <option value="">All Calls</option>
<?php       foreach ($TGS as $TGID => $TGName): ?>
                                <option value="<?=$TGID?>"<?=($TG == $TGID) ? ' selected="selected"' : ''?>><?=$TGName?></option>
<?php       endforeach; ?>
                            </select>
                        </div>
                        <div class="form-group col-lg-4">
                            <label class="form-control-label">Controls</label>
                            <button class="btn btn-success btn-block" type="submit">View</button>
                        </div>
                    </div>
                </form>
                <div class="row">
                    <div class="form-group col-lg-12">
                        <audio preload="none" tabindex="0" controls>
                            <source type="audio/wav" src="http://www.lalarm.com/en/images/silence.wav" />
                            Sorry, your browser does not support HTML5 audio.
                        </audio>
                    </div>
                    <div class="form-group col-lg-12">Click on a row to begin sequential playback, click file size to download</div>
                </div>
            </div>
            <table class="table">
                <thead>
                    <tr>
                        <td>Time</td>
                        <td>Talk Group</td>
                        <td>MHz</td>
                        <td>Size</td>
                    </tr>
                </thead>
                <tbody>
<?php   if (isset($error)): ?>
                    <tr>
                        <th colspan="4"><?=$error?></th>
                    </tr>
<?php   endif;  ?>
<?php   foreach ($files as $FileTime => [$FileName, $FileSize, $TGID, $TIME, $FREQ]):   ?>
                    <tr>
                        <td><?=date("H:i:s", $FileTime)?></td>
                        <td><?=($TGS[$TGID]) ?? $TGID?></td>
                        <td class="<?=$MHz($FREQ / 1000000)?>"><?=sprintf("%3.6f", (float) $FREQ / 1000000)?></td>
                        <td><a href="audio/<?=$date->format('Y/n/j') . '/' . $FileName?>"><?=round($FileSize / 1024)?>k</a></td>
                    </tr>
<?php   endforeach; ?>
                </tbody>
            </table>
        </div>
    </body>
</html>
