#!/bin/bash

# --- User input ---
read -p "Enter number of client terminals: " num_clients

# Validate input
if ! [[ "$num_clients" =~ ^[0-9]+$ ]] || [ "$num_clients" -le 0 ]; then
    echo "Invalid number of clients."
    exit 1
fi

# --- Compilation ---
# Compiles all .c files in current directory
gcc -o server server.c header.c -Wall
gcc -o client client.c header.c -Wall

# --- Ensure execute permissions ---
chmod +x server client

# --- Open server terminal ---
osascript -e "tell application \"Terminal\" to do script \"cd $(pwd); echo 'SERVER'; ./server; \""

# Optional: give server a moment to start
sleep 1

# --- Open client terminals ---
for ((i=1; i<=num_clients; i++)); do
    osascript -e "tell application \"Terminal\" to do script \"cd $(pwd); echo 'CLIENT $i'; ./client; \""
done

