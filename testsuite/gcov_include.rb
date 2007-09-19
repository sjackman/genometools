full_coverage_files = [ "src/libgtcore/array.c",
                        "src/libgtcore/bittab.c",
                        "src/libgtcore/countingsort.c",
                        "src/libgtcore/dynalloc.c",
                        "src/libgtcore/msort.c",
                        "src/libgtext/align.c",
                        "src/libgtext/chseqids_stream.c",
                        "src/libgtext/gff3_parser.c",
                        "src/libgtext/linearalign.c",
                        "src/libgtext/linearedist.c",
                        "src/libgtext/swalign.c",
                        "src/tools/gt_chseqids.c"
                      ]

full_coverage_files.each do |file|
  base = File.basename(file)
  Name "full coverage for #{file}"
  Keywords "gcov"
  Test do
    run "cd #{$cur} && gcov -o obj/#{file} #{file} && cd -"
    run "mv #{$cur}/#{base}.gcov ."
    grep(base+".gcov", "^    #####", true)
  end
end
