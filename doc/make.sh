#!/bin/bash
mv sources _sources
mv static _static
mv images _images
sphinx-build src .
