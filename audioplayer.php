<?php
$FileType = '.wav';

$system = (empty($_GET['system'])) ? NULL : $_GET['system'];
$date   = (isset($_GET['date'])) ? new DateTimeImmutable($_GET['date']) : new DateTimeImmutable();
$tg     = (empty($_GET['tg'])) ? NULL : $_GET['tg'];

$CONFIG = (function (string $configFilePath = './config.json') {
    if (!file_exists($configFilePath))
        return false;

    return json_decode(file_get_contents($configFilePath));
})();

$TGFile = function (string $tgFilePath): array {
    $return = [];

    if (!file_exists($tgFilePath))
        return $return;

    foreach (file($tgFilePath) as $line) {
        [$DEC, $HEX, $Mode, $AlphaTag, $Description, $Tag, $Group, $Priority] = str_getcsv($line);
        $return[$DEC] = $AlphaTag;
    }

    return $return;
};

$TGS = [];
foreach ($CONFIG->systems as $system) {
    $TGS += $TGFile($system->talkgroupsFile);
}

$MHz = function (float $MHz): string {
    switch (TRUE) {
        case $MHz >= 136 AND $MHz <= 174:
            return 'text-primary'; # VHF
        case $MHz >= 380 AND $MHz <= 520:
            return 'text-danger';  # UHF
        case $MHz >= 764 AND $MHz <= 870:
            return 'text-warning'; # 700
        case $MHz >= 896 AND $MHz <= 901:
        case $MHz >= 935 AND $MHz <= 940:
            return 'text-success'; # 800
        default:
            return 'null';
    }
};

$files = [];
try  {
    foreach (new DirectoryIterator("{$CONFIG->captureDir}/{$date->format('Y/n/j')}/") as $file) {
        $EXT = '.' . $file->getExtension();
        if ($EXT != $FileType)
            continue;

        $Basename = $file->getBasename($EXT);
        [$TGID, $TIME, $FREQ] = preg_split('/[-_]/', $Basename);
        if ($TGID != $tg AND $tg !== NULL)
            continue;

        if ($file->getSize() < 1024)
            continue;   # Filtered because they will produce an error when attempting playback.

        $files[$file->getMTime()] = [$Basename, round($file->getSize() / 1024), $TGID, $TIME, $FREQ / 1000000];
    }
    ksort($files);
} catch (UnexpectedValueException $e) {
    $error = 'No directory found for that date.';
}

$path = substr(realpath($CONFIG->captureDir), strlen($_SERVER['DOCUMENT_ROOT']));

?>
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no, viewport-fit=cover">
        <title>Trunk Player</title>
        <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.2/css/bootstrap.min.css" integrity="sha384-Smlep5jCw/wG7hdkwQ/Z5nLIefveQRIY9nfy6xoR1uRYBtpZgI6339F5dgvm/e9B" crossorigin="anonymous">
    </head>
    <body>
        <div class="container">
            <div id="interface">
                <form method="get">
                    <div class="row">
                        <div class="form-group col-lg-4">
                            <label class="form-control-label" for="date">Date</label>
                            <input class="form-control" id="date" name="date" type="date" value="<?=$date->format('Y-m-d')?>" />
                        </div>
                        <div class="form-group col-lg-4">
                            <label class="form-control-label" for="tg">Talk Group</label>
                            <select class="form-control" id="tg" name="tg">
                                <option value="">All Calls</option>
<?php       foreach ($TGS as $TGID => $TGName): ?>
                                <option value="<?=$TGID?>"<?=($tg == $TGID) ? ' selected="selected"' : ''?>><?=$TGName?></option>
<?php       endforeach; ?>
                            </select>
                        </div>
                        <div class="form-group col-lg-4">
                            <label class="form-control-label">Controls</label>
                            <button class="btn btn-outline-success btn-block" type="submit">View</button>
                        </div>
                    </div>
                </form>
                <div class="row">
                    <div class="form-group col-lg-12">
                        <audio preload="none" controls>
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
                        <td class="<?=$MHz($FREQ)?>"><?=sprintf("%3.4f", $FREQ)?></td>
                        <td><a href="<?="{$path}/{$date->format('Y/n/j')}/{$FileName}{$FileType}"?>"><?=$FileSize?>k</a></td>
                    </tr>
<?php   endforeach; ?>
            </table>
        </div>
        <script>
            window.onload = _ => {
                var index = null,
                  sources = document.querySelector('source'),
                    audio = document.querySelector('audio'),
                     list = document.querySelector('tbody'),
                       tr = [...list.getElementsByTagName('tr')];

                list.addEventListener('click', e => {
                    if (tr[index])
                        tr[index].classList.toggle('bg-secondary');

                    tr.forEach((thisRow, i) => {
                        if (e.target.parentElement == thisRow) {
                            index = i;
                            tr[index].classList.toggle('bg-secondary');
                        }
                    });

                    audio.src = tr[index].querySelector('a');
                    audio.load();
                    audio.play().catch(error => {
                        console.log(`${error.name}: ${error.message}`);
                    });
                }, false);

                audio.addEventListener('ended', _ => {
                    tr[index++].classList.toggle('bg-secondary');
                    if (tr[index])
                        tr[index].click();
                }, false);
            }
        </script>
        <style>
            html, body {
                background: #012;
                color: #FFF;
            }
            #interface, audio {
                width: 100%;
            }
            table {
                text-align: center;
            }
            input, select, textarea {
                color: #fff !important;
                background-color: #1a1c22 !important;
                border-color: #434857 !important;
            }
            tbody tr {
                cursor: pointer;
            }
            a {
                color: #FA0;
            }
            a:hover {
                color: #FC0;
            }
            a:active {
                color: #FF0;
            }
        </style>
    </body>
</html>
