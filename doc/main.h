/**
 * \mainpage ns-3 Documentation
 *
 * \section intro-sec Introduction
 * <a href="http://www.nsnam.org/">ns-3</a> documentation is maintained using 
 * <a href="http://www.doxygen.org">Doxygen</a>.
 * Doxygen is typically used for 
 * API documentation, and organizes such documentation across different
 * modules.   This project uses Doxygen for building the definitive
 * maintained API documentation.  Additional ns-3 project documentation 
 * can be found at the
 * <a href="http://www.nsnam.org/documentation/latest">project web site</a>.
 *
 * \section install-sec Building the Documentation
 * 
 * ns-3 requires Doxygen version 1.5.4 or greater to fully build all items,
 * although earlier versions of Doxygen will mostly work.
 * 
 * Type "./waf --doxygen" or "./waf --doxygen-no-build" to build the 
 *  documentation.  The doc/ directory contains
 * configuration for Doxygen (doxygen.conf) and main.h.  The Doxygen 
 * build process puts html files into the doc/html/ directory, and latex 
 * filex into the doc/latex/ directory.
 * 
 * \section module-sec Module overview
 *
 * The ns-3 library is split across many modules organized under the 
 * <b><a href="modules.html">Modules</a></b> tab.
 *     - aodv
 *     - applications
 *     - bridge
 *     - click
 *     - config-store
 *     - core
 *     - csma
 *     - csma-layout
 *     - dsdv
 *     - emu
 *     - energy
 *     - flow-monitor
 *     - internet
 *     - lte
 *     - mesh
 *     - mobility
 *     - mpi
 *     - netanim
 *     - network
 *     - nix-vector-routing
 *     - ns3tcp
 *     - ns3wifi
 *     - olsr
 *     - openflow
 *     - point-to-point
 *     - point-to-point-layout
 *     - propagation
 *     - spectrum
 *     - stats
 *     - tap-bridge
 *     - test
 *     - tools
 *     - topology-read
 *     - uan
 *     - virtual-net-device
 *     - visualizer
 *     - wifi
 *     - wimax
 *
 */
/**
 * \namespace ns3
 * \brief Every class exported by the ns3 library is enclosed in the
 * ns3 namespace.
 */

