#!/bin/bash

# File size for testing (in bytes)
file_size=1048576  # 1 MB

# Number of iterations for benchmarking
iterations=100

# Arrays to store timing measurements
create_times=()
read_times=()

# Function to create a file and measure the time taken
create_file() {
    local filename="./test/$1"
    local start_time=$(date +%s%N)
    dd if=/dev/zero of="$filename" bs="$file_size" count=1 status=none
    local end_time=$(date +%s%N)
    local elapsed_time=$(( (end_time - start_time) / 1000000 ))  # Convert nanoseconds to milliseconds
    create_times+=("$elapsed_time")
    echo "File created: $filename, Time taken: $elapsed_time ms"
}

# Function to read a file and measure the time taken
read_file() {
    local filename="./test/$1"
    local start_time=$(date +%s%N)
    cat "$filename" > /dev/null
    local end_time=$(date +%s%N)
    local elapsed_time=$(( (end_time - start_time) / 1000000 ))  # Convert nanoseconds to milliseconds
    read_times+=("$elapsed_time")
    echo "File read: $filename, Time taken: $elapsed_time ms"
}

# Function to compute summary statistics
compute_summary_statistics() {
    local times=("$@")
    local sum=0
    local count=${#times[@]}
    local min=${times[0]}
    local max=${times[0]}
    local median
    local mean

    # Calculate sum, min, max
    for time in "${times[@]}"; do
        (( sum += time ))
        (( time < min )) && min=$time
        (( time > max )) && max=$time
    done

    # Calculate mean
    mean=$(bc <<< "scale=2; $sum / $count")

    # Calculate median
    sorted_times=($(printf '%s\n' "${times[@]}" | sort -n))
    if (( count % 2 == 0 )); then
        median_index=$(( count / 2 ))
        median=$(bc <<< "scale=2; (${sorted_times[median_index - 1]} + ${sorted_times[median_index]}) / 2")
    else
        median_index=$(( (count + 1) / 2 ))
        median=${sorted_times[median_index - 1]}
    fi

    # Print summary statistics
    echo "Count: $count"
    echo "Mean: $mean ms"
    echo "Median: $median ms"
    echo "Min: $min ms"
    echo "Max: $max ms"
}

# Function to cleanup test files
cleanup_files() {

    cd test/
    rm -f testfile_*
    echo "Test files cleaned up."
}


# Trap and cleanup files before exiting
trap cleanup_files EXIT

# Run benchmarking for file creation
echo "Benchmarking file creation..."
for ((i = 1; i <= iterations; i++)); do
    create_file "testfile_$i"
done

# Run benchmarking for file read
echo "Benchmarking file read..."
for ((i = 1; i <= iterations; i++)); do
    read_file "testfile_$i"
done

# Compute and print summary statistics for file creation times
echo "Summary statistics for file creation times:"
compute_summary_statistics "${create_times[@]}"

# Compute and print summary statistics for file read times
echo "Summary statistics for file read times:"
compute_summary_statistics "${read_times[@]}"