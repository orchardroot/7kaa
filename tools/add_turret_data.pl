#!/usr/bin/perl
#
# Seven Kingdoms: Ancient Adversaries
#
# Copyright 2026 the turret mod authors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# add_turret_data.pl -- appends the FIRM_TURRET (id 11) data records to
# data/RESOURCE/STD.SET and clones the Fort's build-menu icon to F-11 in
# data/RESOURCE/I_BUTTON.RES.
#
# The turret reuses the Fort (CAMP) sprites: its FBUILD record points at
# the CAMP's existing FFRAME/FBITMAP records, so no bitmap data is added.
#
# Idempotent: exits without changes if a TURRET record already exists.
#
# Usage: perl tools/add_turret_data.pl   (from the repo root)

use warnings;
use strict;

use FindBin;
use lib $FindBin::Bin;
use File::Path qw(make_path remove_tree);

use dbf;

my $STD_SET  = "data/RESOURCE/STD.SET";
my $BUTTONS  = "data/RESOURCE/I_BUTTON.RES";
my $WORK     = "scratch/turret_data";

-f $STD_SET or die "run from the repo root: $STD_SET not found";

# pad/align a value into a fixed-width DBF field
sub fld_str { my ($v, $len) = @_; return sprintf("%-*.*s", $len, $len, $v); }
sub fld_num { my ($v, $len) = @_; return sprintf("%*.*s", $len, $len, $v); }

# --------- unpack STD.SET ----------

remove_tree($WORK);
make_path("$WORK/set");
system("perl", "$FindBin::Bin/deresx", $STD_SET, "$WORK/set", ".dbf") == 0
	or die "deresx $STD_SET failed";

my ($firm_file)   = glob("$WORK/set/*-FIRM.dbf");
my ($fbuild_file) = glob("$WORK/set/*-FBUILD.dbf");
$firm_file && $fbuild_file or die "FIRM/FBUILD dbf not found in $STD_SET";

my $firm   = dbf->read_file($firm_file)   or die "cannot read $firm_file";
my $fbuild = dbf->read_file($fbuild_file) or die "cannot read $fbuild_file";

# --------- idempotency check ----------

my $code_f = $firm->get_field("CODE");
for (my $i = 0; $i < $firm->{header}{records}; $i++) {
	my $code = $firm->get_value($i, $code_f);
	if (defined($code) && $code =~ /^TURRET/) {
		print "TURRET record already present; nothing to do\n";
		exit 0;
	}
}

# --------- append FBUILD record (clone of CAMP, renamed) ----------

my $fb_firm_f = $fbuild->get_field("FIRM");
my $camp_build_idx;
for (my $i = 0; $i < $fbuild->{header}{records}; $i++) {
	my $code = $fbuild->get_value($i, $fb_firm_f);
	if (defined($code) && $code =~ /^CAMP/) {
		$camp_build_idx = $i;
		last;
	}
}
defined($camp_build_idx) or die "CAMP record not found in FBUILD";

my @new_build = @{$fbuild->{records}[$camp_build_idx]};
push(@{$fbuild->{records}}, \@new_build);
$fbuild->{header}{records}++;
my $turret_build_recno = $fbuild->{header}{records};    # 1-based recno
$fbuild->set_value($turret_build_recno-1, $fb_firm_f,
	fld_str("TURRET", $fbuild->get_field_len($fb_firm_f)));

# --------- append FIRM record (clone of CAMP, reworked) ----------

my $camp_firm_idx;
for (my $i = 0; $i < $firm->{header}{records}; $i++) {
	my $code = $firm->get_value($i, $code_f);
	if (defined($code) && $code =~ /^CAMP/) {
		$camp_firm_idx = $i;
		last;
	}
}
defined($camp_firm_idx) or die "CAMP record not found in FIRM";

my @new_firm = @{$firm->{records}[$camp_firm_idx]};
push(@{$firm->{records}}, \@new_firm);
$firm->{header}{records}++;
my $t = $firm->{header}{records} - 1;   # 0-based index of the new record

sub set_firm {
	my ($field, $value, $numeric) = @_;
	my $f = $firm->get_field($field);
	$f >= 0 or die "no field $field in FIRM";
	my $len = $firm->get_field_len($f);
	$firm->set_value($t, $f, $numeric ? fld_num($value, $len)
	                                  : fld_str($value, $len));
}

set_firm("CODE",       "TURRET");
set_firm("NAME",       "Turret");
set_firm("SHORTNAME",  "Turret");
set_firm("OVERTITLE",  "");           # no overseer
set_firm("WORKTITLE",  "Soldier");    # garrison workers
set_firm("TERA_TYPE",  "1");
set_firm("ALLKNOW",    "1");
set_firm("LIVEINTOWN", "");
set_firm("HITPOINTS",  150, 1);
set_firm("LINKTOTOWN", "1");
set_firm("SETUP_COST", 300, 1);
set_firm("YEAR_COST",  60, 1);
set_firm("FIRSTBUILD", $turret_build_recno, 1);
set_firm("BUILDCOUNT", 1, 1);

$firm->write_file($firm_file);
$fbuild->write_file($fbuild_file);

# --------- repack STD.SET ----------

system("perl", "$FindBin::Bin/libresx", $STD_SET, "$WORK/set") == 0
	or die "libresx $STD_SET failed";

# --------- clone Fort icon F-5 -> F-11 in I_BUTTON.RES ----------

make_path("$WORK/buttons");
system("perl", "$FindBin::Bin/deresx", $BUTTONS, "$WORK/buttons", ".bin") == 0
	or die "deresx $BUTTONS failed";

my ($f5) = glob("$WORK/buttons/*-F-5.bin");
$f5 or die "F-5 icon not found in $BUTTONS";
if (!glob("$WORK/buttons/*-F-11.bin")) {
	system("cp", $f5, "$WORK/buttons/99990-F-11.bin") == 0 or die "cp failed";
}
system("perl", "$FindBin::Bin/libresx", $BUTTONS, "$WORK/buttons") == 0
	or die "libresx $BUTTONS failed";

print "TURRET data added: FIRM record 11, FBUILD record $turret_build_recno, icon F-11\n";
