package JavaScript::V8::XS;
use strict;
use warnings;
use parent 'Exporter';

use XSLoader;

our $VERSION = '0.000001';
XSLoader::load( __PACKAGE__, $VERSION );

our @EXPORT_OK = qw[];

1;
__END__

=pod

=encoding utf8

=head1 NAME

JavaScript::V8::XS - Perl XS binding for the V8 JavaScript engine

=head1 VERSION

Version 0.000001

=head1 SYNOPSIS

  use JavaScript::V8::XS;
  my $v8 = JavaScript::V8::XS->new();

=head1 SEE ALSO

L<< https://metacpan.org/pod/JavaScript::V8 >>
L<< https://metacpan.org/pod/JavaScript::Duktape::XS >>

=head1 AUTHOR

=over 4

=item * Gonzalo Diethelm C<< gonzus AT cpan DOT org >>

=back

=head1 THANKS

=over 4

=item * Authors of JavaScript::V8

=back

=cut
