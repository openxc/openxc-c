#!/usr/bin/env bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $DIR/..

KERNEL=`uname`
if [ ${KERNEL:0:7} == "MINGW32" ]; then
    OS="windows"
elif [ ${KERNEL:0:6} == "CYGWIN" ]; then
    OS="cygwin"
elif [ $KERNEL == "Darwin" ]; then
    OS="mac"
else
    OS="linux"
    if ! command -v lsb_release >/dev/null 2>&1; then
        # Arch Linux
        if command -v pacman>/dev/null 2>&1; then
            sudo pacman -S lsb-release
        fi
    fi

    DISTRO=`lsb_release -si`
fi

_cygwin_error() {
    echo
    echo "${bldred}Missing \"$1\"${txtrst} - run the Cygwin installer again and select the base package set:"
    echo "    $CYGWIN_PACKAGES"
    echo "After installing the packages, re-run this bootstrap script."
    die
}

if [ $OS == "cygwin" ] && ! command -v tput >/dev/null 2>&1; then
    _cygwin_error "ncurses"
fi

txtrst=$(tput sgr0) # reset
bldred=${txtbld}$(tput setaf 1)
bldgreen=${txtbld}$(tput setaf 2)

die() {
    echo >&2 "${bldred}$@${txtrst}"
    exit 1
}

_wait() {
    if [ -z $CI ]; then
        echo "Press Enter when done"
        read
    fi
}

_install() {
    if [ $OS == "cygwin" ]; then
        _cygwin_error $1
    elif [ $OS == "mac" ]; then
        # brew exists with 1 if it's already installed
        set +e
        brew install $1
        set -e
    else
        if [ -z $DISTRO ]; then
            echo
            echo "Missing $1 - install it using your distro's package manager or build from source"
            _wait
        else
            if [ $DISTRO == "arch" ]; then
                sudo pacman -S $1
            elif [ $DISTRO == "Ubuntu" ]; then
                sudo apt-get update -qq
                sudo apt-get install $1 -y
            else
                echo
                echo "Missing $1 - install it using your distro's package manager or build from source"
                _wait
            fi
        fi
    fi
}


if [ `id -u` == 0 ]; then
    die "Error: running as root - don't use 'sudo' with this script"
fi

CYGWIN_PACKAGES="ncurses, libusb-win32"

if [ $OS == "mac" ] && ! command -v brew >/dev/null 2>&1; then
    echo "Installing Homebrew..."
    ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"
fi

if ! command -v make >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "make"
    elif [ $OS == "mac" ]; then
            echo "Missing 'make' - install the Xcode CLI tools"
	    die
    else
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S base-devel
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install build-essential -y
        fi
    fi
fi

if ! command -v git >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "git"
    elif [ $OS == "mac" ]; then
        _install git
    fi
fi

echo "Updating Git submodules..."

# git submodule update is a shell script and expects some lines to fail
set +e
if ! git submodule update --init --quiet; then
    echo "Unable to update git submodules - try running \"git submodule update\" to see the full error"
    echo "If git complains that it \"Needed a single revision\", run \"rm -rf src/libs\" and then try the bootstrap script again"
    if [ $OS == "cygwin" ]; then
        echo "In Cygwin this may be true (ignore if you know ca-certifications is installed:"
        _cygwin_error "ca-certificates"
    fi
    die
fi
set -e

if ! ld -lusb-1.0 -o /tmp/libusb 2>/dev/null; then
    echo "Installing libusb..."

    if [ $OS == "cygwin" ]; then
        _install "libusb-win32"
    else
        _install "libusb"
    fi
fi

echo "Installing dependencies for running test suite..."

if [ $OS == "cygwin" ] && ! command -v ld >/dev/null 2>&1; then
    _cygwin_error "gcc4"
fi

if [ $OS == "mac" ]; then
    pushd $DEPENDENCIES_FOLDER
    LLVM_BASENAME=clang+llvm-3.2-x86_64-apple-darwin11
    LLVM_FILE=$LLVM_BASENAME.tar.gz
    LLVM_URL=http://llvm.org/releases/3.2/$LLVM_FILE

    if ! test -e $LLVM_FILE
    then
        echo "Downloading LLVM 3.2..."
        download $LLVM_URL $LLVM_FILE
    fi

    if ! test -d $LLVM_BASENAME
    then
        echo "Installing LLVM 3.2 to local folder..."
        tar -xzf $LLVM_FILE
        echo "LLVM 3.2 installed"
    fi

    popd
fi

if ! ld -lcheck -o /tmp/checkcheck 2>/dev/null; then
    echo "Installing the check unit testing library..."

    _install "check"
fi

popd

echo
echo "${bldgreen}All developer dependencies installed, ready to compile.$txtrst"
