1.upto(14) do |i|
  Name "gt cds test #{i}"
  Keywords "gt_cds"
  Test do
    run_test "#{$bin}gt cds -seqfile #{$testdata}gt_cds_test_#{i}.fas #{$testdata}gt_cds_test_#{i}.in"
    run "diff #{$last_stdout} #{$testdata}/gt_cds_test_#{i}.out"
  end
end

1.upto(14) do |i|
  Name "gt cds test #{i} (-usedesc)"
  Keywords "gt_cds usedesc"
  Test do
    run_test "#{$bin}gt cds -usedesc -seqfile #{$testdata}gt_cds_test_#{i}.fas #{$testdata}gt_cds_test_#{i}.in"
    run "diff #{$last_stdout} #{$testdata}/gt_cds_test_#{i}.out"
  end
end

Name "gt cds test (description range)"
Keywords "gt_cds usedesc"
Test do
  run_test "#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_test_descrange.fas " +
           "#{$testdata}gt_cds_test_descrange.in"
  run "diff #{$last_stdout} #{$testdata}/gt_cds_test_descrange.out"
end

Name "gt cds test (multi description)"
Keywords "gt_cds usedesc"
Test do
  run_test "#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_descrange_multi.fas " +
           "#{$testdata}gt_cds_descrange_multi.in"
  run "diff #{$last_stdout} #{$testdata}/gt_cds_descrange_multi.out"
end

Name "gt cds test (multi description fail 1)"
Keywords "gt_cds usedesc"
Test do
  run_test("#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_descrange_multi_fail_1.fas " +
           "#{$testdata}gt_cds_test_descrange.in", :retval => 1)
  grep $last_stderr, "does contain multiple sequences with ID"
end

Name "gt cds test (multi description fail 2)"
Keywords "gt_cds usedesc"
Test do
  run_test("#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_descrange_multi_fail_2.fas " +
           "#{$testdata}gt_cds_test_descrange.in", :retval => 1)
  grep $last_stderr, "does contain multiple sequences with ID"
end

Name "gt cds test (wrong ID)"
Keywords "gt_cds usedesc"
Test do
  run_test("#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_descrange_wrong_id.fas " +
           "#{$testdata}gt_cds_test_descrange.in", :retval => 1)
  grep $last_stderr, "does not contain a sequence with ID"
end

Name "gt cds test (wrong range)"
Keywords "gt_cds usedesc"
Test do
  run_test("#{$bin}gt cds -usedesc -seqfile " +
           "#{$testdata}gt_cds_descrange_wrong_range.fas " +
           "#{$testdata}gt_cds_test_descrange.in", :retval => 1)
  grep $last_stderr, "cannot find sequence ID"
end

Name "gt cds test (-startcodon no -finalstopcodon no)"
Keywords "gt_cds"
Test do
  run_test "#{$bin}gt cds -startcodon no -finalstopcodon no -seqfile " +
           "#{$testdata}U89959_genomic.fas " +
           "#{$testdata}gt_cds_nostartcodon_nofinalstopcodon.in"
  run "diff #{$last_stdout} " +
      "#{$testdata}/gt_cds_nostartcodon_nofinalstopcodon.out"
end

if $gttestdata then
  Name "gt cds bug"
  Keywords "gt_cds"
  Test do
    run_test "#{$bin}gt cds -seqfile #{$gttestdata}cds/marker_region.fas " +
             "#{$gttestdata}/cds/marker_bug.gff3"
    run "diff #{$last_stdout} #{$gttestdata}/cds/marker_bug.out"
  end
end
