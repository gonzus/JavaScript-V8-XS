use strict;
use warnings;
use 5.008009;

use ExtUtils::MakeMaker;
use Path::Tiny;
use JSON::XS;

my @V8_CC_OPTS;
my @V8_CC_DEFS;
my @V8_CC_WRNS;
my @V8_CC_INCD;
my @V8_LD_OPTS;
my @V8_LD_LIBN;
my @V8_LD_LIBD;

# These defaults assume that you have installed the relevant version X of V8
# under /opt/V8/X, where you have directories include, lib and data.
my $V8_DEFAULT_VERSION  = 7;
my $V8_DEFAULT_ROOT_DIR = '/opt/V8';
my $V8_DEFAULT_INCLUDE  = 'include';
my $V8_DEFAULT_LIB      = 'lib';
my $V8_DEFAULT_DATA     = 'data';

# Examples for changing the V8 configuration:
#
#   V8_VERSION=7 perl Makefile.PL                # use V8 version 7 from /opt/V8/7/...
#   V8_ROOT_DIR=/usr/local/V8 perl Makefile.PL   # use V8 version 7 from /usr/local/V8/7...
#   V8_DATA_DIR=/tmp/blobs perl Makefile.PL      # use defaults but get V8 data blobs from /tmp/blobs

my %config;
my $path = path('./config.json');
if ($path && $path->is_file()) {
    my $data = JSON::XS->new->relaxed->decode($path->slurp());
    %config = %$data if $data;
}
my $V8_VERSION  = $ENV{V8_VERSION}  // $config{V8_VERSION}  // $V8_DEFAULT_VERSION;
my $V8_ROOT_DIR = $ENV{V8_ROOT_DIR} // $config{V8_ROOT_DIR} // $V8_DEFAULT_ROOT_DIR;
my $V8_INCLUDE  = $ENV{V8_INCLUDE}  // $config{V8_INCLUDE}  // $V8_DEFAULT_INCLUDE;
my $V8_LIB      = $ENV{V8_LIB}      // $config{V8_LIB}      // $V8_DEFAULT_LIB;
my $V8_DATA     = $ENV{V8_DATA}     // $config{V8_DATA}     // $V8_DEFAULT_DATA;

my $V8_BASE_DIR = $ENV{V8_BASE_DIR} // join('/', $V8_ROOT_DIR, $V8_VERSION);

my $V8_INCLUDE_DIR = $ENV{V8_INCLUDE_DIR} // join('/', $V8_BASE_DIR, $V8_INCLUDE);
my $V8_LIB_DIR     = $ENV{V8_LIB_DIR}     // join('/', $V8_BASE_DIR, $V8_LIB);
my $V8_DATA_DIR    = $ENV{V8_DATA_DIR}    // join('/', $V8_BASE_DIR, $V8_DATA);

push @V8_CC_INCD, $V8_INCLUDE_DIR;
push @V8_LD_LIBD, $V8_LIB_DIR;

push @V8_CC_OPTS, qw< -O2 -std=c++14 >;
push @V8_CC_WRNS, qw< all extra no-unused-parameter >;
push @V8_LD_LIBN, qw< v8 v8_libbase v8_libplatform icuuc icui18n >;

push @V8_CC_DEFS, join('=', 'V8_DATA_DIR', $V8_DATA_DIR);

if ($^O eq 'linux') {
    push @V8_CC_DEFS, qw< PERL_JAVASCRIPT_V8_XS_LINUX >;

    # these apply only on linux, where the V8 build has to ignore standard
    # include files and instead looks at its own internal libc++ files
    push @V8_CC_OPTS, qw< -nostdinc++ >;
    push @V8_CC_INCD, map { "$V8_INCLUDE_DIR/$_/include" } qw< libc++ libc++abi >;
    push @V8_CC_DEFS, qw< _LIBCPP_ABI_UNSTABLE >;
    push @V8_CC_DEFS, join('=', '_LIBCPP_ABI_VERSION', 'Cr');

}

if ($^O eq 'darwin') {
    push @V8_CC_DEFS, qw< PERL_JAVASCRIPT_V8_XS_DARWIN >;
}

my $CC = 'c++';
my $LD = 'c++';

my $CC_OPTS = join(' ',
    (@V8_CC_OPTS),
    (map { "-D$_" } @V8_CC_DEFS),
    (map { "-W$_" } @V8_CC_WRNS),
    (map { "-I$_" } @V8_CC_INCD),
);
my $LD_OPTS = join(' ',
    (@V8_LD_OPTS),
    (map { "-L$_" } @V8_LD_LIBD),
    (map { "-l$_" } @V8_LD_LIBN),
    (map { "-Wl,-rpath,$_" } @V8_LD_LIBD),
);

WriteMakefile(
    NAME           => 'JavaScript::V8::XS',
    VERSION_FROM   => 'lib/JavaScript/V8/XS.pm',
    ABSTRACT_FROM  => 'lib/JavaScript/V8/XS.pm',
    LICENSE        => 'mit',
    MIN_PERL_VERSION => 5.018000,
    PREREQ_PM      => {
        'JSON::PP'        => 0,
        'Text::Trim'      => 0,
        'XSLoader'        => 0,
        'ExtUtils::XSpp'  => '0.11',
    },
    TEST_REQUIRES  => {
        'Data::Dumper'    => 0,
        'JSON::PP'        => 0,
        'Path::Tiny'      => 0,
        'Scalar::Util'    => 0,
        'Test::More'      => 0,
        'Test::Output'    => 0,
        'Test::Exception' => 0,
        'Text::Trim'      => 0,
        'Time::HiRes'     => 0,
    },
    AUTHOR         => [
        'Gonzalo Diethelm (gonzus@cpan.org)',
    ],

    # compiler settings
    CC                 => "$CC $CC_OPTS",
    DEFINE             => '',
    depend             => { 'v8-perl.c' => 'JavaScript-V8-XS-Context.xsp' },

    # linker settings
    LD                 => "$LD $LD_OPTS",
    OBJECT             => '$(O_FILES)',

    # precompiler settings
    XSOPT              => '-C++ -hiertype',
    TYPEMAPS           => ['perlobject.map'],

    # repo settings
    META_MERGE         => {
        'meta-spec'    => { version => 2 },
        resources      => {
            repository => {
                type   => 'git',
                url    => 'git@github.com:gonzus/JavaScript-V8-XS',
                web    => 'https://github.com/gonzus/JavaScript-V8-XS',
            },
        },
    },
);
