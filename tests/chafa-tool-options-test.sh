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

# Force palettes are checked only after effective mode and format resolution.
force_ok () {
    "$tool" --probe off --animate off -s 1 --colors full "$@" \
        "${top_srcdir}/tests/data/good/pixel.png" >/dev/null || exit $?
}
force_bad () {
    if "$tool" --probe off --animate off -s 1 --colors full "$@" \
        "${top_srcdir}/tests/data/good/pixel.png" >/dev/null 2>&1; then
        echo "Unexpectedly accepted force palette: $*" >&2
        exit 1
    fi
}
force_ok --force-color-fg '001-003,#112233' --force-color-bg 232-255
force_ok --colors 16 --force-color-fg 1 --fg 1 --force-color-bg 4 --bg 4
force_ok --colors 16/8 --force-color-fg 8-15 --force-color-bg 0-7
force_ok --colors 240 --force-color-fg black,red
force_ok --colors 8 --force-color-fg 200 --force-color-fg 1
force_ok --colors 16 --force-color-fg 1 --bg '#123456'
force_ok --colors 16 --force-color-fg 1 --bg 1 --invert
force_ok --force-color-fg 9 --fg-only --colors 16/8
force_bad --colors 16 --force-color-fg red
force_bad --colors 16 --force-color-fg 16
force_bad --colors 16/8 --force-color-bg 8
force_bad --colors 240 --force-color-bg 0
force_bad --colors 16 --force-color-bg 0 --bg '#123456'
force_bad --colors 16 --force-color-bg 0 --fg '#123456' --invert
force_bad --colors 2 --force-color-fg 1
force_bad --colors none --force-color-bg 0
force_bad --force-color-bg 0 --fg-only
force_bad --force-color-fg 1 --format kitty
force_bad --force-color-fg 1 --format sixel
force_bad --force-color-bg 1 --format iterm
force_bad --force-color-fg '1,,2'
force_bad --force-color-bg ''
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
