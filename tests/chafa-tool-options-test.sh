#!/bin/sh

[ "x${srcdir}" = "x" ] && srcdir="."
. "${srcdir}/chafa-tool-test-common.sh"

for option in --bg --fg; do
    for color in 0 1 01 001 15 16 231 232 255 000001 000255 112233; do
        "$tool" "$option=$color" --version >/dev/null || exit $?
    done
    for color in 256 999 0256 0001 00001; do
        if "$tool" "$option=$color" --version >/dev/null 2>&1; then
            echo "Unexpectedly accepted $option=$color" >&2
            exit 1
        fi
    done
done

color_spaces="rgb din99d"
color_extractors="average median"
dither_types="none ordered diffusion"
symbol_selectors="none all wide 0..1ffff"

for format in $formats; do
    for color_space in $color_spaces; do
        for color_extractor in $color_extractors; do
            for dither_type in $dither_types; do
                for main_symbols in $symbol_selectors; do
                    for fill_symbols in $symbol_selectors; do
                        run_cmd "$tool -f $format -c 240 -s 53 --threads 12 --animate no --color-space $color_space \
--color-extractor $color_extractor --dither $dither_type --symbols $main_symbols --fill $fill_symbols" || exit $?
                    done
                done
            done
        done
    done
done
