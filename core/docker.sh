#!/bin/sh

# Define the list of valid services
valid_services="ALL BASE SIP ASR LMS"

# Check if at least 2 arguments are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 SERVICE DOCKER_COMMAND [ADDITIONAL_OPTIONS]"
    exit 1
fi

# Check if the provided service is valid
SERVICE="$1"
if ! echo "$valid_services" | grep -wq "$SERVICE"; then
    echo "Invalid SERVICE. Please choose one of: ALL, BASE, SIP, ASR, LMS."
    exit 1
fi

# Capture the first two arguments as SERVICE and DOCKER_COMMAND
shift 1
DOCKER_COMMAND="$@"

# Function to generate a list of -f arguments for Docker Compose based on .yml files in the directory
generate_yaml_args() {
    local args=""
    for yaml_file in *.yml; do
        if [ -f "$yaml_file" ]; then
            args="$args -f $yaml_file"
        fi
    done
    echo "$args"
}

# Start the baseline service if the SERVICE is "BASE"
if [ "$SERVICE" = "BASE" ]; then
    docker compose --env-file ./SERVICE.env -f common $DOCKER_COMMAND
# Check if the SERVICE is "UNINSTALL" to stop and remove all containers
elif [ "$SERVICE" = "ALL" ]; then
    yaml_args="$(generate_yaml_args)"
    if [ -z "$yaml_args" ]; then
        echo "No .yml files found in the directory."
        exit 1
    fi
    docker compose --env-file ./SERVICE.env -f common $yaml_args $DOCKER_COMMAND
else
    # Check if the SERVICE.yml file exists
    if [ ! -f "${SERVICE}.yml" ]; then
        echo "The ${SERVICE}.yml file does not exist."
        exit 1
    fi

    # Start the specified service
    docker compose --env-file ./SERVICE.env -f common -f "${SERVICE}.yml" $DOCKER_COMMAND
fi