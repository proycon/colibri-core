#!/bin/bash
mv sources _sources
mv static _static
mv images _images
$VIRTUAL_ENV/bin/sphinx-build src .
