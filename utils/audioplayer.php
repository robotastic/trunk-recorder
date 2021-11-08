<?php
$FileType = '.m4a';

$system = (empty($_GET['system'])) ? null : $_GET['system'];
$date   = (isset($_GET['date'])) ? new DateTimeImmutable($_GET['date']) : new DateTimeImmutable();
$tg     = (empty($_GET['tg'])) ? null : $_GET['tg'];

$CONFIG = (function (string $configFilePath = './config.json') {
    if (!file_exists($configFilePath)) {
        return false;
    }

    return json_decode(file_get_contents($configFilePath));
})();

if (false === $CONFIG) {
    $error = 'Config file does not exist';
    goto html;
}

if (empty($CONFIG->systems)) {
    $error = 'No systems found in config file';
    goto html;
}

$TGFile = function (?string $tgFilePath): array {
    $return = [];

    if (!file_exists($tgFilePath)) {
        return $return;
    }

    $radioreference_format = false;
    foreach (file($tgFilePath) as $line) {
        if (!$radioreference_format)
        {
            [$DEC, $HEX, $Mode, $AlphaTag, $Description, $Tag, $Group, $Priority] = str_getcsv($line);
        }
        else
        {
            [$DEC, $HEX, $AlphaTag, $Mode, $Description, $Tag, $Group] = str_getcsv($line);
        }
        if ($DEC == 'Decimal')
        {
            $radioreference_format = true;
            continue;
        }
        $return[$DEC] = $AlphaTag;
    }

    return $return;
};

$TGS = [];
foreach ($CONFIG->systems ?? [] as $system) {
    $TGS[$system->shortName] = $TGFile($system->talkgroupsFile);
}

$files = [];
try {
    foreach ($CONFIG->systems ?? [] as $system) {
        foreach (new DirectoryIterator("{$CONFIG->captureDir}/{$system->shortName}/{$date->format('Y/n/j')}/") as $file) {
            $EXT = '.' . $file->getExtension();
            if ($EXT != $FileType) {
                continue;
            }

            $Basename = $file->getBasename($EXT);
            [$TGID, $TIME, $FREQ] = preg_split('/[-_]/', $Basename);

            if ($TGID != $tg and $tg !== null) {
                continue;
            }

            if ($file->getSize() < 1024) {
                continue;
            }   # Filtered because they will produce an error when attempting playback.

            $files[$TIME . $FREQ] = [$Basename, round($file->getSize() / 1024), $TGID, $TIME, $FREQ / 1000000, $system->shortName];
        }
    }
    ksort($files);
} catch (UnexpectedValueException $e) {
    $error = 'No directory found for that date.';
}

html:
?>
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
        <meta name="format-detection" content="telephone=no">
        <title>Trunk Player</title>
        <link href="https://stackpath.bootstrapcdn.com/bootswatch/4.5.0/flatly/bootstrap.min.css" rel="stylesheet" integrity="sha384-mhpbKVUOPCSocLzx2ElRISIORFRwr1ZbO9bAlowgM5kO7hnpRBe+brVj8NNPUiFs" crossorigin="anonymous">
        <link href="https://stackpath.bootstrapcdn.com/bootswatch/4.5.0/flatly/bootstrap.min.css" rel="stylesheet" integrity="sha384-mhpbKVUOPCSocLzx2ElRISIORFRwr1ZbO9bAlowgM5kO7hnpRBe+brVj8NNPUiFs" crossorigin="anonymous" media="(prefers-color-scheme: light)">
        <link href="https://stackpath.bootstrapcdn.com/bootswatch/4.5.0/darkly/bootstrap.min.css" rel="stylesheet" integrity="sha384-Bo21yfmmZuXwcN/9vKrA5jPUMhr7znVBBeLxT9MA4r2BchhusfJ6+n8TLGUcRAtL" crossorigin="anonymous" media="(prefers-color-scheme: dark)">
        <style>
            #interface, audio {
                width: 100%;
            }
            table {
                text-align: center;
            }
        </style>
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
<?php   foreach ($CONFIG->systems as $system):  ?>
                                <optgroup value="<?=$system->shortName?>">
<?php       foreach ($TGS[$system->shortName] as $TGID => $TGName): ?>
                                <option value="<?=$TGID?>"<?=($tg == $TGID) ? ' selected="selected"' : ''?>><?=$TGName?></option>
<?php       endforeach; ?>
                                </optgroup>
<?php   endforeach; ?>
                            </select>
                        </div>
                        <div class="form-group col-lg-4">
                            <label class="form-control-label">Controls</label>
                            <button class="btn btn-primary btn-block" type="submit">Filter</button>
                        </div>
                    </div>
                </form>
                <div class="row">
                    <div class="form-group col-lg-12"><button class="btn btn-primary btn-block" onclick="window.scrollTo(0, document.body.scrollHeight);">Jump to bottom</button>Click on a row to begin sequential playback. Click file size to download.</div>
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
                    <tr class="text-warning">
                        <th colspan="4"><?=$error?></th>
                    </tr>
<?php   endif;  ?>
<?php   foreach ($files ?? [] as $FileTime => [$FileName, $FileSize, $TGID, $TIME, $FREQ, $SHORTNAME]):   ?>
                    <tr>
                        <td><?=date("H:i:s", $TIME)?></td>
                        <td><?=($TGS[$SHORTNAME][$TGID]) ?? $TGID?></td>
                        <td><?=sprintf("%3.4f", $FREQ)?></td>
                        <td><a href="<?="{$CONFIG->captureDir}/{$SHORTNAME}/{$date->format('Y/n/j')}/{$FileName}{$FileType}"?>"><?=$FileSize?>k</a></td>
                    </tr>
<?php   endforeach; ?>
            </table>
            <br />
            <br />
            <br />
            <br />
            <nav class="navbar fixed-bottom navbar-expand-sm navbar-dark bg-primary">
                <audio preload="none" controls>
                    Sorry, your browser does not support HTML5 audio.
                </audio>
            </nav>
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
                        tr[index].classList.toggle('table-active');

                    tr.forEach((thisRow, i) => {
                        if (e.target.parentElement == thisRow) {
                            index = i;
                            tr[index].classList.toggle('table-active');
                        }
                    });

                    audio.src = tr[index].querySelector('a');
                    audio.load();
                    audio.play().catch(error => {
                        console.log(`${error.name}: ${error.message}`);
                    });
                }, false);

                audio.addEventListener('ended', _ => {
                    tr[index++].classList.toggle('table-active');
                    if (tr[index])
                        tr[index].click();
                }, false);

            }
        </script>
    </body>
</html>
