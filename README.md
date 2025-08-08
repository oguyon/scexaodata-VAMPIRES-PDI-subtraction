# scexaodata-VAMPIRES-PDI-subtraction
Polarization Differential Imaging (PDI) subtraction

Raw data location: /mnt/sdata/2_ARCHIVED_DATA/20241219/vgen2


```bash
# Compile
mkdir build
cd build
cmake ..
make
cd -

# Download to local data directory, reading csv file
./build/csv_reader

# Extract FITS headers (requires fitsheader)
find . -maxdepth 2 -type f -wholename "*.fits.fz" -print0 | xargs -0 -I{} bash -c 'fitsheader {} > {}.txtheader'
```

