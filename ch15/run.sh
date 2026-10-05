#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

if [ ! -f bochsrc.disk ]; then
    echo "bochsrc.disk not found"
    exit 1
fi
if [ ! -x ./bin/bochs ]; then
    echo "./bin/bochs is missing or not executable"
    exit 1
fi
if [ ! -f hd10M.img ]; then
    echo "hd10M.img not found; run make all first"
    exit 1
fi
if [ ! -f hd50M.img ]; then
    echo "hd50M.img not found; run make all first"
    exit 1
fi

echo -e "n\np\n1\n2048\n50000\nn\ne\n2\n51200\n101807\nn\n53248\n101807\nw\n" | fdisk hd50M.img >/dev/null 2>&1
./bin/bochs -f bochsrc.disk
