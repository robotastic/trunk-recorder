#!/usr/bin/env bash
#
# Uploads trunk-recorder files to Google cloud storage bucket
#
# Assumes you have already installed and configured gsutil
# See <https://cloud.google.com/storage/docs/gsutil>
#
# Must have execute permission ('chmod +x unload_gcs.sh') and
# be located in the same directory as the trunk-recorder
# executable.
#
# Configure GSUTIL and BUCKET to your environment
#
GSUTIL="/usr/bin/gsutil"
BUCKET="p25-player-audio"   # add path to BUCKET as needed (e.g. "bucket-name/subdir/")
#
DELAY="2s"
TIMEFORMAT="+%F %T.%6N"
FILENAME="$1"
# echo "Uploading: ${FILENAME}"
BASENAME="${FILENAME%.*}"
JSON="${BASENAME}.json"
sleep ${DELAY}
${GSUTIL} -q cp ${FILENAME} ${JSON} gs://${BUCKET}
if [ $? -eq 0 ]
then
    echo "[$(date "${TIMEFORMAT}")] Uploaded: ${BASENAME} Bucket: ${BUCKET}"
else
    echo "[$(date "${TIMEFORMAT}")] Upload failed: ${BASENAME} Bucket: ${BUCKET}"
    exit 1
fi
