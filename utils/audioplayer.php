<?php
// no leading period
$FileType = 'm4a';

/**
*   This allows you to remove the leading portion of a directory name, so you can use
*   absolute paths in the config file, and not have to match that structure in your web server
*
*   For example, if your recordings are in `/home/trunkrecorder/audio_files`, you'd set this to 
*   "/home/trunkrecorder/", and define a location block in the nginx config like so:
*    location /audio_files/ {
*        root /home/trunkrecorder/;
*    }
*   Which would let you put your document root anywhere, and not require serving up the entire
*   trunk-recorder directory.
*/
$base_directory_name = '/home/trunkrecorder';

$CONFIG = (function (string $configFilePath = './../configs/config.json') {
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
        $return[$DEC] = ['tag' => $AlphaTag, 'mode' => $Mode];
    }

    return $return;
};

$TGS = [];
foreach ($CONFIG->systems ?? [] as $system) {
    $TGS[$system->shortName] = $TGFile($system->talkgroupsFile);
}

if (isset($_REQUEST['since']))
{
    $filter_date = (isset($_GET['date'])) ? new DateTimeImmutable($_GET['date']) : new DateTimeImmutable();
    $filter_tg = (empty($_GET['tg'])) ? null : $_GET['tg'];

    $latest_file = 0;
    $files = [];
    try {
        foreach ($CONFIG->systems ?? [] as $system)
        {
            $directory = new RecursiveDirectoryIterator("{$CONFIG->captureDir}/{$system->shortName}/");
            $iterator = new RecursiveIteratorIterator($directory);
            foreach ($iterator as $file)
            {
                if ($file->getExtension() != $FileType)
                {
                    continue;
                }
                // parse filename to retrieve info about the call
                $basename = $file->getBaseName('.'.$FileType);
                [$TGID, $TIME, $FREQ] = preg_split('/[-_]/', $basename);

                if ($filter_tg !== null && $TGID != $filter_tg)
                {
                    continue;
                }

                if ($filter_date->format('Y-m-d') != strftime('%F', $TIME))
                {
                    continue;
                }

                if ($file->getSize() < 1024)
                {
                    // Filtered because they will produce an error when attempting playback.
                    continue;
                }   

                $http_path = $file->getPath().'/'.$file->getFileName();
                if ($base_directory_name != '')
                {
                    $http_path = str_ireplace($base_directory_name, '', $http_path); 
                }

                if (isset($TGS[$system->shortName][$TGID]['mode']) && stripos($TGS[$system->shortName][$TGID]['mode'], 'E') !== false)
                {
                    // current version of trunk recorder has an issue where it still tries to record encrypted calls
                    // filter out any talk group that's listed as permanently encrypted to avoid corrupted audio
                    continue;
                }

                $files[$TIME.$FREQ] = [
                    'path'       => $http_path,
                    'size_kb'    => round($file->getSize() / 1024),
                    'talkgroup'  => ($TGS[$system->shortName][$TGID]['tag']) ?? $TGID,
                    'unix_date'  => $TIME,
                    'date'       => strftime('%F %T', $TIME),
                    'frequency'  => ($FREQ / 1000000),
                    'systemname' => $system->shortName,
                ];

                if ($TIME > $latest_file) $latest_file = $TIME;
            }
        }
        ksort($files);
    } catch (UnexpectedValueException $e) {
        $error = 'No directory found for that date.';
    }

    $newfiles = [];
    foreach ($files as $details)
    {
        if ($details['unix_date'] > $_REQUEST['since'])
        {
            $newfiles[] = $details;
        }
    }

    // on initial page laod, limit to the 100 most recent files
    // don't filter if we're filtering to a specific TG
    if ($_REQUEST['since'] == 0 && $filter_tg === null)
    {
        $newfiles = array_slice($newfiles, -100);
    }

    Header('Content-Type: application/json');
    echo json_encode([
        'latest'   => $latest_file,
        'newfiles' => $newfiles,
    ]);
    exit();
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
        <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/select2/4.0.13/css/select2.min.css" integrity="sha512-nMNlpuaDPrqlEls3IX/Q56H36qvBASwb3ipuo3MxeWbsQB1881ox0cRv7UPTgBlriqoynt35KjEwgGUeUXIPnw==" crossorigin="anonymous" referrerpolicy="no-referrer" />

        <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.6.0/jquery.min.js" integrity="sha512-894YE6QWD5I59HgZOGReFYm4dnWc1Qt5NtvYSaNcOP+u1T9qYdvdihz0PPSiiqn/+/3e7Jo4EaG7TubfWGUrMQ==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
        <script src="https://cdnjs.cloudflare.com/ajax/libs/handlebars.js/4.7.7/handlebars.min.js" integrity="sha512-RNLkV3d+aLtfcpEyFG8jRbnWHxUqVZozacROI4J2F1sTaDqo1dPQYs01OMi1t1w9Y2FdbSCDSQ2ZVdAC8bzgAg==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
        <script src="https://cdnjs.cloudflare.com/ajax/libs/select2/4.0.13/js/select2.min.js" integrity="sha512-2ImtlRlf2VVmiGZsjm9bEyhjGW4dU7B6TNwh/hx/iSByxNENtj3WVE6o/9Lj4TJeVXPi4bnOIMXFIJJAeufa0A==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
        <style>
            #interface, audio {
                width: 100%;
            }
            table {
                text-align: center;
            }
            .select2-selection {
                height: calc(1.5em + .75rem + 2px) !important;
            }            
        </style>
    </head>
    <body>
        <div class="container">
            <div id="interface">
                <div class="row">
                    <div class="form-group col-lg-4">
                        <label class="form-control-label" for="date">Date</label>
                        <input class="form-control" id="date" name="date" type="date" value="<?=strftime('%F')?>" />
                    </div>
                    <div class="form-group col-lg-4">
                        <label class="form-control-label" for="tg">Talk Group</label>
                        <select class="form-control" id="tg" name="tg">
                            <option value="">All Calls</option>
<?php   foreach ($CONFIG->systems as $system):  ?>
                            <optgroup value="<?=$system->shortName?>">
<?php       foreach ($TGS[$system->shortName] as $TGID => $data): ?>
                            <option value="<?=$TGID?>"><?=$data['tag']?> (<?=$TGID?>, <?=$data['mode']?>)</option>
<?php       endforeach; ?>
                            </optgroup>
<?php   endforeach; ?>
                        </select>
                    </div>
                    <div class="form-group col-lg-4">
                        <label class="form-control-label">Controls</label>
                        <button class="btn btn-primary btn-block" type="button" onclick="updateFiles(true)">Filter</button>
                    </div>
                </div>
                <div class="row">
                    <div class="form-group col-lg-12"><button class="btn btn-primary btn-block" onclick="window.scrollTo(0, document.body.scrollHeight);">Jump to bottom</button>Click on a row to begin sequential playback. Click file size to download.</div>
                </div>
            </div>
            <table class="table" id="calls_table">
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
            </table>
            <br />
            <br />
            <br />
            <br />
            <nav class="navbar fixed-bottom navbar-expand-sm navbar-dark bg-primary">
                <audio id="audio_player" preload="none" controls>
                    Sorry, your browser does not support HTML5 audio.
                </audio>
            </nav>
        </div>
        <script>
            var latest = 0;
            var template = Handlebars.compile(`
                <tr>
                    <td>{{ date }}</td>
                    <td>{{ talkgroup }}</td>
                    <td>{{ frequency }}</td>
                    <td><a href="{{ path }}">{{ size_kb }}k</a></td>
                </tr>
            `);
            var last_played = 0;
            var auto_play_new_row = false;

            $(function () {
                updateFiles();
                setInterval(updateFiles, 10*1000);
                $('#tg').select2();

                $('#calls_table tr').on('click', onClickTableRow);

                $('#audio_player').on('ended', function () {
                    var current_row = $('#calls_table .table-active')[0];

                    var next_row = $(current_row).closest('tr').next('tr');
                    if (next_row.length > 0)
                    {
                        playAudioFromRow(next_row[0]);
                    }
                    else
                    {
                        auto_play_new_row = true;
                    }
                });
            });

            function updateFiles(clear_files=false)
            {
                if (clear_files)
                {
                    $('#audio_player').trigger('stop');
                    $('#calls_table tr').remove();
                    latest = 0;     
                }
                $.ajax({
                    url: window.location.pathname,
                    data: {
                        'since': latest,
                        'tg': $('#tg').val(),
                        'date': $('#date').val(),
                    },
                    success: function (data, textStatus, jqXHR) {
                        if (data.latest == 0 || data.newfiles.length == 0) return;

                        $(data.newfiles).each(function (i, newcall) {
                            var new_row_html = template(newcall);
                            var new_row_ref = $(new_row_html).appendTo($('#calls_table tbody'));
                            if (auto_play_new_row)
                            {
                                auto_play_new_row = false;
                                playAudioFromRow(new_row_ref);
                            }
                        });
                        latest = data.latest;

                        // refresh click handlers to account for new rows
                        $('#calls_table tr').off();
                        $('#calls_table tr').on('click', onClickTableRow);
                    }
                });
            }

            function onClickTableRow()
            {
                playAudioFromRow(this);
            }

            function playAudioFromRow(row)
            {
                $('#calls_table .table-active').removeClass('table-active');
                $(row).addClass('table-active');

                var dllink = $($(row).find('a')[0])
                $('#audio_player').attr('src', dllink.attr('href'));
                $('#audio_player').trigger('play');                
            }
        </script>
    </body>
</html>
