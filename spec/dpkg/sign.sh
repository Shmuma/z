#!/bin/sh

find . -iname '*.changes' -exec debsign '{}' ';'