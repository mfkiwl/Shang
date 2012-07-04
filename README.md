[The Shang High-level Synthesis Framework](http://sysueda.github.com/Shang/)
========================================

Overview
--------

The Shang [high-level synthesis](http://en.wikipedia.org/wiki/High-level_synthesis) framework, which is implemented as an [LLVM backend](http://llvm.org/docs/WritingAnLLVMBackend.html), take as input C specification and generates [Verilog RTL hardware desciption](http://en.wikipedia.org/wiki/Verilog) from LLVM IR.
Unlike most other LLVM-based high-level synthesis frameworks,
e.g. [C-to-Verilog](http://www.c-to-verilog.com/) or [Legup](http://legup.eecg.utoronto.ca/),
which work on the LLVM-IR layer, Shang works on the LLVM target-independent code generator,
this allow Shang to easily represent and optimize some high-level synthesis specific operation(instruction), e.g. reduction OR, concatenation, etc.

At the moment, Shang has several high-level synthesis specific (optimization) passes including:

*  (pre-schedule) Arithmetic/bitwise operation strength reduction
*  BasicBlock frequency based hyper-block formation
*  Pre-schedule logic synthesis with [ABC](http://www.eecs.berkeley.edu/~alanmi/abc/) (optional, maps all bitwise logic operations to look-up tables)
*  Scheduling pass that support Multi-cycles chaining
*  Unified register/functional-unit allocation and binding
*  [Register-transfer level](http://en.wikipedia.org/wiki/Register-transfer_level) optimizations, e.g. common subexpression elimination by and-invert graph (AIG) based structural hashing.
*  Verilog RTL code generation pass

Users can also schedule some "scripting passes" which apply [Lua](http://www.lua.org/about.html) scripts to the design to accomplish tasks inluding:
*  Generate vendor specific timing constraints for the design, this is necessary because Shang will always generates multi-cycles pathes (by the so-called Multi-cycles Chaining).
*  Generate platform specific bus interface to allow the design generated by Shang cooperates with others components in the same system.
*  ...

Getting Start
-------------
This guide should quickly get you started using Shang to synthesize C into Verilog. We divided this guide into 2 parts. Installation will guide you to install required packages and complie the source code on Ubuntu. Main or Hybrid Flow will teach you how to use Shang in different synthesis flow. This method has been tested on the ubuntu-10.04.3, we assume that you are using a Linux 32/64-bit environment, we have not tested Shang on Windows or Mac OS.
###  1.Installation ###
####   Required Packages or Softwares on Ubuntu ####
<dl>
<dt>To install the required packages on Ubuntu run:</dt>
  <pre>
  sudo apt-get install update tcl8.5-dev dejagnu expect gxemul texinfo \
  build-essential liblpsolve55-dev libgmp3-dev automake libtool python-all-dev \
  lua5.1 git-core gitk git-gui cmake cmake-gui timeout
  </pre>
<dt>To install systemc</dt>
Download the source code from http://www.accellera.org/downloads/standards/systemc, when you do this you need an account to access Workspace, you can register as member employee to get an account. Among all the file provided by Accellera Systems Initiative there is a file called INSTALL which will guide you to install systemc on your computer whether for windows or unix.
<dt>To install verilator</dt>
You can download the verilator-3.833.tgz from the website provided as follow (http://www.veripool.org/wiki/verilator/Installing).On the webpage it alse told you the way to install verilator on your computer, you just do as it said.
<dt>To install clang</dt>
Manually download the latest version of clang from http://llvm.org and add it to your path. For 32-bit machines:
<pre>
wget http://llvm.org/releases/2.9/clang+llvm-2.9-i686-linux.tgz
tar xvzf clang+llvm-2.9-i686-linux.tgz
export PATH=$PWD/clang+llvm-2.9-i686-linux/bin:$PATH
</pre>
<dt>To install the Quartus and Modelsim on Ubuntu</dt>
You will need Modelsim to simulate the Verilog and Quartus to synthesize the Verilog for an FPGA. You can download Quartus Web Edition and Modelsim for free from Altera 
(https://www.altera.com/download/software/quartus-ii-we).
<pre>
bash ~/soft/10.1sp1_quartus_free_linux.sh
bash ~/soft/10.1_modelsim_ase_linux.sh
</pre>
After installing Quartus update your environment to add quartus and modelsim to your path:
<pre>export PATH=~/altera/10.1/modelsim_ase/bin/:~/altera/10.1sp1/quartus/bin/$PATH</pre>
<dt>Note</dt>
<dd>You must edit the path above to point to your particular Quartus installation location. Shang has been tested with Quartus 10.1sp1.</dd>
</dl>

####   Compile Shang Source and run the testsuite ####
<dl>  
<dt>Download (https://github.com/etherzhhb/Shang) and compile the Shang source:</dt>
First you should download the source code of llvm to the local computer, you’ll get a folder named llvm, cd into the ~\llvm\lib\Target folder and rename the verilogbackend of Shang which was called VBE to VerilogBackend. Now you have got all the source code of Shang.
To get both llvm and VBE you can run those:
<pre>
git clone  http://llvm.org/git/llvm.git
cd llvm/lib/Target/
git clone https://github.com/SysuEDA/Shang.git
</pre>
Before compiling Shang, you need to checkout LLVM revsion 16436dffb50fac4677c7162639f8da0b73eb4e99, and a patch located in <path-to-Shang's-source>/util/0001-Minimal-patch-to-llvm-3.1svn.patch. You can do this by the below commands:
<pre>
cd path-to-llvm-source
git reset --hard 16436dffb50fac4677c7162639f8da0b73eb4e99
git apply path-to-Shang's-source/util/0001-Minimal-patch-to-llvm-3.1svn.patch
</pre>
Then we use CMake ,a cross-platform, open-source build system to control the compilation process of Shang. On command line you can type the cmake command shows as follew to configure path for the environment variable used by Shang.
<pre>
cmake ../llvm/ 
-DLUA_INCLUDE_DIR=/usr/include/lua5.1/ 
-DLUA_LIBRARY=/usr/lib/liblua5.1.a 
-DLUA_LUAC=/usr/bin/luac5.1           
-DLUA_BIN2C=/home/kun/local/bin2c5.1 
-DLUABIND_INCLUDE_DIR=/usr/include 
-DLUABIND_LIBRARY=/usr/lib/libluabind.so 
-DLPSOLVE_INCLUDE_DIR=/home/kun/local/include/ 
-DLPSOLVE_LIBRARY=/home/kun/local/lib/liblpsolve55.so 
-DENABLE_LOGIC_SYNTHESIS=ON 
-DABC_INCLUDE_DIR=/home/kun/alanmi-abc-5dead10b1fe1 
-DABC_LIBRARY=/home/kun/alanmi-abc-5dead10b1fe1/libabc.a 
-DSYSTEMC_ROOT_DIR=/home/kun/local/systemc-2.2.0/ 
-DFRONTEND=/home/kun/local/llvm_offical_build/bin/clang 
-DQUARTUS_BIN_DIR=/opt/altera/10.1/quartus/bin 
-DVERILATOR_ROOT_DIR=/home/kun/local/verilator/
</pre>
<dt>Note</dt>
<dd>In the command you should specified the relative path after “cmake” to point to the folder named llvm we have got from the remote repository. The environment variable alse should be specified absolute path on where the package installed or lib placed.</dd>
</dl>
<dl>
<dt>Run the testsuite</dt>
By running the testsuite you can verify your installation and checking the correction of verilog file converted by Shang. 
You can synthesis or simulate not only all the c file in CHStone but alse the specific c file at once.
</dl>
<dl>
In order to simulate all benchmark file,you can run:
<pre>
cd shang-build
make benchmark_test
</pre>
In order to simulate and synthesis all benchmark file,you can run:
<pre>
cd shang-build
make benchmark_report
</pre>
Using float64_add.c as example,you can alse simulate the specific c file like this:
<pre>
cd shang-build
make float64_add_IMS_ASAP_diff_output
</pre>
</dl>
###  2.Main or Hybrid Flow ###
As main flow, Shang can compile an entire C program to hardware. It can alse compile user designated functions to hardware while remaining program segments are executed in software on the Altera Nios II Soft Processor. This is referred to as the hybrid flow.
<dl>
<dt>Main flow</dt>
For example, let’s synthesis the float64_add.c of the dfadd CHStone benchmark, you can run as this:
<pre>
cd shang-build
make float64_add_IMS_ASAP_main_hls
</pre>
you will get the Verilog file float64_add_IMS_ASAP_main_DUT_RTL.v in shang-build/lib/Target/VerilogBackend/testsuite/benchmark/ChStone/dfadd/float64_add_IMS_ASAP_main/
<dt>Hybrid flow</dt>
For example, let’s synthesis the float64_add.c of the dfadd CHStone benchmark, you can run as this:
<pre>
cd shang-build
make float64_add_IMS_ASAP_hls
</pre>
you will get the Verilog file float64_add_IMS_ASAP_DUT_RTL.v in shang-build/lib/Target/VerilogBackend/testsuite/benchmark/ChStone/dfadd/float64_add_IMS_ASAP/
</dl>

Writing Lua script
------------------
Shang uses the popular scripting language Lua (version 5.1) as the configure input.   

[Lua](http://www.lua.org/about.html) is a powerful, fast, lightweight, embeddable scripting language. 
If you are not familiar with the syntax of Lua, you should spend a little time and go over the [Lua 5.1 reference book](http://www.lua.org/manual/5.1/).

We will demonstrate how to write a Lua script to configure Shang as follows. Now we assume that you want to convert
a C code float64_add.c which is available at testsuite\benchmark\ChStone\dfadd into the corresponding RTL code.  
1.  Setup the input and output path   
    Now we supposed that we wirte a Lua script named "configure.lua" for Shang. To begin with, You should assign the
    input path of .bc or .ll file (float64_add.bc). Here we assume that the path is (D:/float64_add/). 
    We also presume that the output path is the same as the input path. We output the RLT code (float64_add.v)
    and timing constraint code(float64_add.sdc). a simple example:   
        <pre><code>InDir = [[D:/float64_add/]]
        OutDir = Indir
        InputFile = InDir .. 'float64_add.bc'
        RTLOutput = OutDir .. 'float64_add.v'
        SDCOutput = OutDir .. 'float64_add.sdc'</pre></code>   
2.  Setup the convert function  
    If we want to convert certain function (float64_add in this case) into hardware, we should have the following statement in the Lua script.  

        Functions.float64_add = { ModName = float64_add,
                                  Scheduling = SynSettings.ASAP,
                                  Pipeline = SynSettings.DontPipeline }  
In this table, we create a table in which the "ModName" is the name of the converted verilog module, the "Scheduling" is the
schedule mode of Shang(ASAP or ILP etc.), the "Pipeline" is the option whether we use pipeline in Shang.  
3.  Setup the platform information script.  
    Supposed that we use the EP2C35F672C6 FPGA of altera, we could create another lua script named "EP2C35F672C6.lua" to
    hold the platform information of EP2C35F672C6. The "EP2C35F672C6.lua" could be like this:   

        local FMAX = 100
        PERIOD = 1000.0 / FMAX
        FUs.ClkEnSelLatency = 1.535 / PERIOD --1.535
        FUs.MaxAllowedMuxSize = 8
        FUs.RegCost = 64
        FUs.LUTCost = 64
        FUs.MaxLutSize = 4
        FUs.MaxMuxPerLUT = 2
        FUs.LutLatency = 0.635 / PERIOD
        -- Latency table for EP2C35F672C6
        FUs.AddSub = { Latencies = { 1.994 / PERIOD, 2.752 / PERIOD, 4.055 / PERIOD, 6.648 / PERIOD },
                       Costs = {128, 576, 1088, 2112, 4160}, StartInterval=1,
            		         ChainingThreshold = ADDSUB_ChainingThreshold}
        FUs.Shift  = { Latencies = { 3.073 / PERIOD, 3.711 / PERIOD, 5.209 / PERIOD, 6.403 / PERIOD },
                       Costs = {64, 1792, 4352, 10176, 26240}, StartInterval=1,
        			         ChainingThreshold = SHIFT_ChainingThreshold}
        FUs.Mult   = { Latencies = { 2.181 / PERIOD, 2.504 / PERIOD, 6.503 / PERIOD, 9.229 / PERIOD },
                       Costs = {64, 4160, 8256, 39040, 160256}, StartInterval=1,
        			         ChainingThreshold = MULT_ChainingThreshold}
        FUs.ICmp   = { Latencies = { 1.909 / PERIOD, 2.752 / PERIOD, 4.669 / PERIOD, 7.342 / PERIOD },
                       Costs = {64, 512, 1024, 2048, 4096}, StartInterval=1,
        			         ChainingThreshold = ICMP_ChainingThreshold}
        
        FUs.MemoryBus = { Latency= 0.5, StartInterval=1, AddressWidth=POINTER_SIZE_IN_BITS, DataWidth=64 }
        
        FUs.BRam = {  Latency=1, StartInterval=1, DataWidth = 64, InitFileDir = TEST_BINARY_ROOT, Template=[=[
        
        // Block Ram $(num)
        reg                      bram$(num)we;
        reg   [$(addrwidth - 1):0]   bram$(num)addr;
        reg   [$(datawidth - 1):0]   bram$(num)in;
        reg   [$(datawidth - 1):0]   bram$(num)out;
        
        reg   [$(datawidth - 1):0]  mem$(num)[0:$(2^addrwidth-1)];
        
        #if filename ~= [[empty]] then 
        initial
          begin
            $readmemh("$(filepath)$(filename)", mem$(num));
          end
        #end
        
        always @ (posedge $(clk)) begin
          if (bram$(num)en) begin
            if (bram$(num)we)
              mem$(num)[bram$(num)addr] <= bram$(num)out;
        
            bram$(num)in <= mem$(num)[bram$(num)addr];
          end
        end
        ]=]}  
In this script, we configure the period of corresponding paltform and parameter of function units. We make the latency
table for the combinational logic.(to be continued...)   
In configure.lua, we have to include the EP2C35F672C6.lua with the following statement:   
    <pre><code>-- load platform information script
    dofile(InDir .. 'EP2C35F672C6.lua')</pre></code>   
4.  Setup the other configuration which is not to be concerned by client  
    User will find the other configuration in the example configure.lua at the XXXX.    
As a whole the example configure.lua should look like this(without the configuration information mentioned in setp 4):  

    -- Setup the input and output path.
    InDir = [[D:/float64_add/]]
    OutDir = Indir
    InputFile = InDir .. 'float64_add.bc'
    RTLOutput = OutDir .. 'float64_add.v'
    SDCOutput = OutDir .. 'float64_add.sdc''
    
    -- Setup the function to convert and synthesis mode.
    Functions.float64_add = { ModName = float64_add,
                              Scheduling = SynSettings.ASAP,
                              Pipeline = SynSettings.DontPipeline }
                                  
    -- Load platform information script
    dofile(InDir .. 'EP2C35F672C6.lua')  
        
Internal Representations
------------------------
To be written.

Todo
----
*  Transcational-level optimization
