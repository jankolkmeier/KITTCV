#include "CircleMarkerDetection.h"

struct SearchState {
    int whites;
    int blacks;
    int state;
    int changes;
    int start_idx;
    int end_idx;

    SearchState() {
        whites = 0;
        blacks = 0;
        state = 0;
        changes = 0;
        start_idx = 0;
        end_idx = 0;
    }
};


// 1 [Read blacks]
//    Flop: 2 / possible_start
// 2 [Read whites]
//    Flop_U: 3  (U: ratio blacks/whites is higher *many blacks*)
// 3 [Read black Ring]
//    Flop_S: 4
//    Flop_N: 2
//    NoFlop: 1
// 4 [Read white Ring]
//    Flop_S_X: 1 [Done] / definite_end
//    Flop_S: 3
//    Flop_N: 1
//    NoFlop: 2

bool _search(Mat & row, int r, vector<Point3i> & bars, bool horizontal) {
    bool res = false;
    float max_ratio = 0.75f;
    uchar marker_flips = 8;

    int end = horizontal ? row.size().width : row.size().height;

    uchar flips = 0;
    uchar state = 1;
    ushort possible_start = 0;
    ushort definite_end = 0;
    ushort curr = 0;
    ushort prev = 0;

    bool black = (uchar)row.at<uchar>(0, 0) == 0;
    bool white = !black;

    for (int i = 0; i < end; i++) {
        bool next_white = horizontal ? (uchar)row.at<uchar>(0, i) == 255 : (uchar)row.at<uchar>(i, 0) == 255;

        if (next_white == white) curr++;
        white = next_white;
        black = !white;

        float ratio = max(curr, prev)/(float)(curr+prev);

        if (state == 1) { // Read blacks
            if (white) {
                state = 2;
                // cout << "2A";
                prev = curr;
                curr = 1;
                possible_start = i;
            }
        } else if (state == 2) { // Read whites
            if (black) {
                if (curr < prev) {// flaky: && ratio > max_ratio
                    state = 3;
                    // cout << "3A";
                    flips = 1;
                } else {
                    state = 1;
                    // cout << "1A";
                }
                prev = curr;
                curr = 1;
            }
        } else if (state == 3) { // Read black ring
            if (white) {
                prev = curr;
                curr = 1;
                if (ratio < max_ratio) {
                    state = 4;
                    // cout << "4A";
                    flips++;
                } else {
                    state = 2;
                    // cout << "2B";
                }
            } else if (curr > prev && ratio >= max_ratio) {
                state = 1;
                // cout << "1B";
            }
        } else if (state == 4) { // Read white ring
            if (black) {
                definite_end = i-1;

                prev = curr;
                curr = 1;
                if (ratio < max_ratio) {
                    if (flips == marker_flips) {
                        res = true;
                        int size = (definite_end - possible_start)/2;
                        int center = possible_start+size;
                        Point3i current(horizontal ? center : r, horizontal ? r : center, size);
                        bool found = false;
                        // cout << "|C: " << current.x << "," << current.y << "|" << endl;
                        for (int b = 0; b < bars.size(); b++) {
                            Point3i * other = &bars[b];
                            if (abs(current.x - other->x) < current.z && abs(current.y - other->y) < other->z) {
                                other->x = (current.x + other->x)/2;
                                other->y = (current.y + other->y)/2;
                                other->z = max(current.z, other->z);
                                found = true;
                                break;
                            }
                        }
                        if (!found) bars.push_back(current);
                    } else {
                        state = 3;
                        // cout << "3B";
                        flips++;
                    }
                } else {
                    state = 1;
                    // cout << "1C";
                }
            } else if (curr > prev && ratio >= max_ratio) {
                state = 2;
                //cout << "2C";
            }
        }
        //if (curr == 1)
        //    cout << (white ? " W " : " B ");
    }
    return res;
}

void search(Mat & img, vector<Point3i> & circles) {
    Size size = img.size();
    vector<Point3i> horiz_bars;

    // Look for (grouped) bars in rows
    for (int r = 0; r < size.height; r++) {
        Mat rMat = img.row(r);
        _search(rMat, r, horiz_bars, true);
    }

    // For each horizontal bar(group), check if vertical bar(group) exists
    for (int b = 0; b < horiz_bars.size(); b++) {
        Point3i bar = horiz_bars[b];
        vector<Point3i> vert_bars;
        vert_bars.push_back(bar);
        bool found = false;

        int range = bar.z*2;
        // we only search in area around bar
        for (int c = max(0, bar.x-range); c < min(size.width, bar.x+range); c++) {
            Mat rMat = img.col(c).rowRange(max(0, bar.y-range), min(size.height, bar.y+range));
            if (_search(rMat, c, vert_bars, false)) {
                found = true;
            }
        }
        if (found) {
            circles.push_back(vert_bars.at(0));
        }
    }
}
