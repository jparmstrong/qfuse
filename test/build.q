
system "rm -Rf hdb segments; mkdir -p hdb; mkdir -p segments";

pwd:raze system "pwd";
`:hdb/par.txt 0: ((pwd,"/segments/par_"),/:)string 1+til 3;

gen_t:{([]a:`$(20 12)#count[.Q.nA]?.Q.nA; b:(20 12)#count[.Q.nA]?.Q.nA; c:-20?20)};

dates:{{distinct raze (min x;-20?x;max x)}.z.D-til 60};

{[tbl;dt] 
    -1 .Q.s1 ("Saving ";tbl;dt); 
    sym:`$"sym_",string tbl;
    tbl set $[tbl=`foo;`a`f`g xcol;::] gen_t[];
    .Q.dpfts[`:hdb;dt;`a;tbl;sym];
 }.'raze (`foo,/:dates[];`bar,/:dates[]);

baz:`a`h`i xcol gen_t[];
`:hdb/baz/ set .Q.ens[`:hdb;baz;`sym_baz];
-1 "Saving baz";  