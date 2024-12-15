#!/bin/bash

##############################CONFIGURATION##############################
enable_email_alerts="false" #Choose whether to enable email alerts when Trunk-Recorder Crashes ("true" or "false") 
email_address="" #Email address alerts should be sent to (if enabled)

# Email alerts are disabled by default. If you don't want to use them, no further action is needed.
# If email alerts are enabled, make sure to configure your system's email service (e.g., Mutt, Mail, Postfix, Sendmail, or an SMTP relay service like Gmail).
# This script uses "mutt" email service by default. Adjust accordingly if you use another service.
# If emails are enabled, I recommend increasing the 'sleep' time (line 98). This will send an alert every time trunk-recorder fails to restart. (If set to 120 seconds, you could get an email every 2 minutes)
##############################CONFIGURATION##############################

##START OF SCRIPT##

# Get the directory of the current script (used for locating your config)
script_dir="$(cd "$(dirname "$0")" && pwd)"

# Path to the config.json file
config_file="$script_dir/config.json"

# Function to parse JSON and extract a value for a given key
get_json_value() {
    local key=$1
    jq -r ".$key // empty" "$config_file"
}

# Parse the config.json file
if [[ -f "$config_file" ]]; then
    log_file_enabled=$(get_json_value "logFile")
    log_dir=$(get_json_value "logDir")
else
    log_file_enabled="false"
fi

# Determine the log directory (used to attach the most recent log file when TR crashes)
if [[ "$log_file_enabled" == "true" ]]; then
    if [[ -n "$log_dir" ]]; then
        # Use the specified logDir location
        log_path="$log_dir"
    else
        # Use the default /logs directory within the script's directory
        log_path="$script_dir/logs"
    fi
else
    log_path=""
fi

# Infinite loop to keep the script running
while true; do
    # Run trunk-recorder
    ./trunk-recorder

    # Capture the exit code
    exit_code=$?

    # Log the crash information to the console
    echo "Trunk-Recorder crashed with exit code $exit_code. Respawning.." >&2

    # Check if emails are enabled
    if [[ "$enable_email_alerts" == "true" ]]; then
        # Check if logs are enabled
        if [[ "$log_file_enabled" == "true" && -n "$log_path" ]]; then
            # Find the most recent log file in the log directory
            recent_log=$(ls -t "$log_path" 2>/dev/null | head -n 1)
            recent_log_path="$log_path/$recent_log"

            # Check if the most recent log file exists
            if [[ -f "$recent_log_path" ]]; then
                # Convert the log file to a plain text file
                converted_log="${recent_log_path}.txt"
                cp "$recent_log_path" "$converted_log"

                # Get the last 10 lines of the log file
                last_lines=$(tail -n 10 "$recent_log_path")
                
                # Send an email with the text file attached and the last 10 lines in the body using mutt
                echo -e "Trunk-Recorder crashed!\n\nExit Code: $exit_code.\n\nLast 10 log entries:\n$last_lines" | \
                mutt -e "set content_type=text/plain" -s "Trunk-Recorder crashed!" -a "$converted_log" -- "$email_address"

                # Clean up the temporary text file
                rm -f "$converted_log"
            else
                # If no log file exists, send an email without an attachment using mutt
                echo -e "Trunk-Recorder crashed!.\n\nExit Code: $exit_code.\n\n No logs attached (No logs found!)" | \
                mutt -e "set content_type=text/plain" -s "Trunk-Recorder crashed!" -- "$email_address"
            fi
        else
            # Logs are not enabled, send an email without an attachment using mutt
            echo -e "Trunk-Recorder crashed!.\n\nExit Code: $exit_code.\n\n No logs available to attach (logging is disabled)" | \
            mutt -e "set content_type=text/plain" -s "Trunk-Recorder crashed!" -- "$email_address"
        fi
    else
        # Emails are disabled, just log to console and try to restart
        echo "Trunk-Recorder crashed with exit code $exit_code. Respawning without sending an email."
    fi

    # Wait for the configured duration before restarting the program
    sleep 120
done
