# unit-script.h

Creates a list of radio IDs in a file named "radiolist.csv" located in the
shortName directory along side the recordings, and logs radio activity to
"radiolog.csv" files located in each day's recordings directory

Make sure to fill in the CAPTUREDIR="" line with the path used in config.json, no ending /

file format: radioID,timestamp,action,talkgroup,
for radiolist.csv, acknowledgment response timestamps are added at the end
when they are seen after a different action

Feel free to customize the script; to use for multiple systems, include in
each system's config.json section

NOTE: You need to run "echo > radiolist.csv" where the file(s) is going to be
beforehand as sed doesn't work on empty files, and to capture actions before
trunk-recorder makes the daily directory upon recording the first call, set
up a cron task of: 0 0 * * * mkdir -p <capturedir>/$(date +\%Y/\%-m/\%-d/)

sed usage based on https://stackoverflow.com/a/49852337
