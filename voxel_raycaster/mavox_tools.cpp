#include "mavox_tools.h"

using namespace std;

int parseMavoxHeader(ifstream& input, int& version, size_t& gridlength, size_t& nfilled){
	string line; input >> line;  // #mavox
	if (line.compare("#mavox") != 0) {cout << "  Error: first line reads [" << line << "] instead of [#mavox]" << endl; return 0;}
	input >> version;

	size_t depth, height, width; // should all be equal - mavox only supports cubic grids
	depth = -1;
	nfilled = -1;
	bool done = false;

	while(input.good() && !done) {
		input >> line;
		if (line.compare("data") == 0) done = true; // when we encounter data keyword, we're at the end of the ASCII header
		else if (line.compare("dim") == 0) {input >> depth >> height >> width;}
		else if (line.compare("filled") == 0) {input >> nfilled;}
		else { cout << "  unrecognized keyword [" << line << "], skipping" << endl;
			char c; do { c = input.get(); } while(input.good() && (c != '\n'));
		}
	}
	if (!done) {cout << "  error reading header" << endl; return 0;}
	if (depth == -1) {cout << "  missing dimensions in header" << endl; return 0;}
	if (nfilled == -1) {cout << "  missing filled voxel counts in header" << endl; return 0;}
	if((depth != height) || (depth != width) || (width != height)){
		cout << "  not a cubic grid" << endl; return 0;
	}
	gridlength = depth;
	return 1;
}

int readMavoxFile(std::string const filename, Octree*& tree){
	Timer t = Timer();

	// Open file for reading
	cout << "Reading " << filename << " ..." << endl; cout.flush();
	ifstream input = ifstream(filename.c_str(), ios::in); // open filestream for input

	// Parse Mavox header
	int version;
	size_t gridlength,nfilled;
	parseMavoxHeader(input,version,gridlength,nfilled);
	cout << "  mavox version: " << version << endl;
	cout << "  grid size: "<< gridlength << "x" << gridlength << "x" << gridlength << " voxels" << endl;
	double percentfilled = ((double) nfilled / (double) (gridlength*gridlength*gridlength))*100.0f;
	cout << "  filled voxels: "<< nfilled << " (" << percentfilled << " %)" << endl;

	// Initialize octree builder
	OctreeBuilder builder = OctreeBuilder(gridlength,nfilled);

	// Read data and pass it to octree builder
	int voxels_read = 0; // we're only reading the amount of voxels 
	uint64_t morton_number;
	float nx,ny,nz;
	cout << "  reading mavox voxel data ... "; cout.flush();
	while(input.good() && voxels_read < nfilled){
		input >> morton_number >> nx >> ny >> nz;
		vector<uint64_t> dec = mortonDecode(morton_number);
		size_t rx = dec[2]; size_t ry = dec[1]; size_t rz = dec[0];
		float r = (float(rx)/gridlength)*25;float g = (float(ry)/gridlength)*25;float b = (float(rz)/gridlength)*25;
		DataPoint point = DataPoint(1,RGBColor(r,g,b),vec3(nx,ny,nz));
		builder.addDataPoint(morton_number,point);
		voxels_read++;
	}
	cout << "Done." << endl;
	// Finalize octree and get it
	builder.finalizeOctree();
	tree = builder.getOctree();
	cout << "  finished in " << t.getTimeMilliseconds() << " milliseconds" << endl;
	return 1;
};