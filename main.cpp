#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <fstream>
#include <vector>
#include <math.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("CodeBlocksWindowsApp");

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("Code::Blocks Template Windows App"),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */



struct Point
{
    int x,y;

    Point(int x=0, int y=0)
    {
        this->x = x;
        this->y = y;
    }
};

HDC hdc;
PAINTSTRUCT ps;
vector< pair< vector<Point> ,int> > SavedShapes;
vector<Point> points;
int command = -1;
bool load;

class Vector4
{

    public:

        double V[4];

        Vector4(double x=0, double y=0, double z=0, double w=0)
        {
            V[0] = x;
            V[1] = y;
            V[2] = z;
            V[3] = w;
        }
        double& operator[](int index)
        {
            return V[index];
        }
        double operator*(Vector4& V1)
        {
            double ret = 0;
            for(int i=0; i<4; ++i)
            {
                ret += this->V[i] * V1[i];
            }
            return ret;
        }
};

#define MAXENTRIES 600
struct Entry
{
    int xmin,xmax;
};

class Matrix4
{
    public:

        Vector4 M[4];

        Matrix4(double H[])
        {
            memcpy(M , H, 16*sizeof(double));
        }
        Vector4& operator[](int index)
        {
            return M[index];
        }
        Vector4 operator*(Vector4 &V) // Multiplication of matrix and vector
        {
            Vector4 ret;

            for(int i=0; i<4; ++i)
            {
                for(int j=0; j<4; ++j)
                {
                    ret[i] += this->M[i][j] * V[j];
                }
            }
            return ret;
        }

};

struct Code
{
    bool left, right, top, bottom; // 0 -> inside Clipping region, 1 otherwise
    Code(){
        left = right = top = bottom = 0;
    }

    void setCode(Point p, int xLeft, int xRight, int yTop, int yBottom)
    {

        if(p.x < xLeft)
            this->left = 1;
        else
            this->left = 0;

        if(p.x > xRight)
            this->right = 1;
        else
            this->right = 0;

        if(p.y < yTop)
            this->top = 1;
        else
            this->top = 0;

        if(p.y > yBottom)
            this->bottom = 1;
        else
            this->bottom = 0;

    }

    bool isOut()
    {
        return (left | right | top | bottom);
    }
};

void InitEntries(Entry table[])
{
    for(int i=0; i<MAXENTRIES; i++)
    {
        table[i].xmin = (int)1e9;
        table[i].xmax = -(int)1e9;
    }
}

void ScanEdge(Point v1, Point v2, Entry table[])
{
    if(v1.y == v2.y)
        return;

    if(v1.y > v2.y)
        swap(v1,v2);

    double minv = (double)(v2.x-v1.x) / (v2.y-v1.y);

    while(v1.y < v2.y)
    {
        if(v1.x < table[v1.y].xmin)
            table[v1.y].xmin = (int)ceil(v1.x);
        if(v1.x>table[v1.y].xmax)
            table[v1.y].xmax = (int)floor(v1.x);
        v1.y++;
        v1.x += minv;
    }
}

void DrawScanLines(Entry table[])
{
    for(int y=0; y < MAXENTRIES; y++)
    {
        if(table[y].xmin < table[y].xmax)
        {
            for(int x=table[y].xmin; x <= table[y].xmax; x++)
            {
                SetPixel(hdc, x, y, RGB(0,255,0));
            }
        }
    }
}

void ConvexFill()
{
    Entry *table = new Entry[MAXENTRIES];

    InitEntries(table);

    int n = 5;
    Point v1 = points[n-1];

    for(int i=0; i<n; i++)
    {
        Point v2 = points[i];
        ScanEdge(v1, v2, table);
        v1 = points[i];
    }

    DrawScanLines(table);

    delete table;
}

void draw(Point p1, Point p2)
{
    MoveToEx(hdc, p1.x, p1.y, NULL);
    LineTo(hdc, p2.x, p2.y);
}

Point vIntersect(Point s, Point e, int x)
{
    Point p;
    p.x = x;
    p.y = s.y +(x - s.x) * (e.y - s.y) / (e.x - s.x);
    return p;


}
Point hIntersect(Point s, Point e, int y)
{
    Point p;
    p.y = y;
    p.x = s.x + (y - s.y) * (e.x - s.x) / (e.y - s.y);
    return p;

}

Vector4 getCoeff(double x0, double s0, double x1, double s1)
{
    static double H[16] = {2,1,-2,1,-3,-2,3,-1,0,1,0,0,1,0,0,0};
    static Matrix4 basis(H);
    Vector4 v(x0, s0, x1, s1);

    return (basis * v);
}



void go();

void Save(){

    fstream shapes;
    shapes.open("shapes.txt", fstream::in | fstream::out | fstream::trunc);
    for(int i=0;  i<SavedShapes.size(); ++i)
    {
        shapes << SavedShapes[i].second << "*";
        for(int j=0; j<SavedShapes[i].first.size(); ++j)
        {
            shapes << SavedShapes[i].first[j].x << "," << SavedShapes[i].first[j].y;
            if (j == SavedShapes[i].first.size()-1)
                shapes << "/";
            else
                shapes << "-";
        }
    }
    SavedShapes.clear();
    shapes.close();
    cout << "Saved\n";
}

void Load(){
    fstream shapes;
    shapes.open("shapes.txt", fstream::in | fstream::out);
    load = true;

    string line;
    while ( !shapes.eof())
    {
        getline ( shapes, line, '/');

        stringstream ss(line);
        string c;
        getline(ss, c, '*');
        command = atoi(c.c_str());
        if (command >= 3 && command <= 9)
        {
            Point p1, p2;
            string x1, y1, x2, y2;
            getline(ss, x1, ',');
            getline(ss, y1, '-');
            getline(ss, x2, ',');
            getline(ss, y2);

            p1.x = atoi(x1.c_str());
            p1.y = atoi(y1.c_str());
            p2.x = atoi(x2.c_str());
            p2.y = atoi(y2.c_str());

            points.push_back(p1), points.push_back(p2);
        }
        else
        {
            Point p1, p2, p3, p4;
            string x1, y1, x2, y2, x3, y3, x4, y4;
            getline(ss, x1, ',');
            getline(ss, y1, '-');
            getline(ss, x2, ',');
            getline(ss, y2, '-');
            getline(ss, x3, ',');
            getline(ss, y3, '-');
            getline(ss, x4, ',');
            getline(ss, y4, '-');

            p1.x = atoi(x1.c_str());
            p1.y = atoi(y1.c_str());
            p2.x = atoi(x2.c_str());
            p2.y = atoi(y2.c_str());
            p3.x = atoi(x3.c_str());
            p3.y = atoi(y3.c_str());
            p4.x = atoi(x4.c_str());
            p4.y = atoi(y4.c_str());

            points.push_back(p1), points.push_back(p2);
            points.push_back(p3), points.push_back(p4);
        }

        go();

    }
    load = false;
    cout << "Loaded\n";

    shapes.close();
}

void MidPointLine(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    if(xs > xe){
        swap(xs, xe);
        swap(ys, ye);
    }


    int dx = xe-xs, dy = ye-ys;
    double m = (double)dx/dy;

    double xMid, yMid, xInc, yInc;

    /* the Four cases*/

    if(0<m && m<1){
        xMid = 1, yMid = 0.5;
        xInc = 1, yInc = 1;
    } else if (m>=1){
        xMid = 0.5, yMid = 1;
        xInc = 1, yInc = 1;
    } else if (m<=-1){
        xMid = 0.5, yMid = -1;
        xInc = 1, yInc = -1;
    } else if (0>m && m>-1){
        xMid = 1, yMid = -0.5;
        xInc = 1, yInc = -1;
    }

    if(abs(dx) > abs(dy))
    {

        for(int x=xs, y=ys; x<xe; x+=xInc)
        {

            SetPixel(hdc, x, y, RGB(0,255,0));

            double d = (y+yMid-ys)*dx - (x+xMid-xs)*dy;

            if(d > 0)
                y+=yInc;

        }

    }
    else
    {
        for(int x=xs, y=ys; y<ye; y+=yInc)
        {

            SetPixel(hdc, x, y, RGB(0,255,0));

            double d = (y+yMid-ys)*dx - (x+xMid-xs)*dy;

            if(d < 0)
                x+=xInc;

        }
    }
}

void DirectNaiveLine(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    if(xs > xe){
        swap(xs, xe);
        swap(ys, ye);
    }

    double m = (double)(ye-ys)/(xe-xs);

    for(double x=xs; x<=xe; x+=0.5){
        int y = round(ys + (x-xs) * m);
        SetPixel(hdc, x, y, RGB(255,0,0));
    }

}

void SimpleDDALine(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    if(xs > xe){
        swap(xs, xe);
        swap(ys, ye);
    }

    int dx = xe-xs, dy = ye-ys;

    double m = (double)dy/dx;

    double xInc, yInc;

    if (abs(m) < 1){
        xInc = dx>0?1:-1;
        yInc = m * xInc;
    } else {
        yInc = dy>0?1:-1;
        xInc = 1/m * yInc;
    }

    for(double x=xs, y=ys; x<xe; x+=xInc, y+=yInc){
        SetPixel(hdc, x, round(y), RGB(255,0,0));
    }
}

void CartesianCircle(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    int dx = abs(xe-xs), dy = abs(ye-ys);
    int R = sqrt((dx*dx) + (dy*dy));

    int x=0, y=R;

    while(x < y){

        x++;
        y = round(sqrt((double)(R*R-x*x)));

        SetPixel(hdc, xs+x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys-x, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys-x, RGB(255,0,0));

    }
}

void PolarCircle(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    int dx = abs(xe-xs), dy = abs(ye-ys);
    int R = sqrt((dx*dx) + (dy*dy));
    double theta=0, dtheta=1.0/R;
    int x=R, y=0;

    while(x > y){

        theta+=dtheta;

        x=round(R*cos(theta));
        y=round(R*sin(theta));


        SetPixel(hdc, xs+x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys-x, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys-x, RGB(255,0,0));

    }

}

void MidPointCircle(Point s, Point e){

    int xs = s.x, ys = s.y;
    int xe = e.x, ye = e.y;

    int dx = abs(xe-xs), dy = abs(ye-ys);
    int R = sqrt((dx*dx) + (dy*dy));
    int d = 1-R;

    int x=0, y=R;

    while(x < y){

        if(d < 0)
            d+=2*x+2;
        else
            d+=2*(x-y)+5, y--;
        x++;
        SetPixel(hdc, xs+x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys+y, RGB(255,0,0));
        SetPixel(hdc, xs-x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+x, ys-y, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys+x, RGB(255,0,0));
        SetPixel(hdc, xs-y, ys-x, RGB(255,0,0));
        SetPixel(hdc, xs+y, ys-x, RGB(255,0,0));

    }
}

void HermitCurve(Point p1, Point t1, Point p2, Point t2){

    MoveToEx(hdc, p1.x, p1.y, NULL);
    LineTo(hdc, t1.x, t1.y);
    MoveToEx(hdc, p2.x, p2.y, NULL);
    LineTo(hdc, t2.x, t2.y);

    Vector4 xCoeff = getCoeff(p1.x, t1.x, p2.x, t2.x);
    Vector4 yCoeff = getCoeff(p1.y, t1.y, p2.y, t2.y);

    int numPoints = 15;
    double dt = 1.0/numPoints;

    MoveToEx(hdc, p1.x, p1.y, NULL);

    for(double t=0; t<=1; t+=dt)
    {

        Vector4 v;
        v[3] = 1;

        for(int i=2; i>=0; --i)
        {
            v[i] = v[i+1] * t;
        }

        int x = round(xCoeff * v);
        int y = round(yCoeff * v);
        LineTo(hdc, x, y);

    }

}

void BezierCurve(Point p1, Point p2, Point p3, Point p4){
    Point t1(3*(p2.x-p1.x),3*(p2.y-p1.y));
    Point t2(3*(p4.x-p3.x),3*(p4.y-p3.y));

    MoveToEx(hdc, p1.x, p1.y, NULL);
    LineTo(hdc, p2.x, p2.y);
    MoveToEx(hdc, p3.x, p3.y, NULL);
    LineTo(hdc, p4.x, p4.y);

    Vector4 xCoeff = getCoeff(p1.x, t1.x, p4.x, t2.x);
    Vector4 yCoeff = getCoeff(p1.y, t1.y, p4.y, t2.y);

    int numPoints = 15;
    double dt = 1.0/numPoints;

    MoveToEx(hdc, p1.x, p1.y, NULL);

    for(double t=0; t<=1; t+=dt)
    {

        Vector4 v;
        v[3] = 1;

        for(int i=2; i>=0; --i)
        {
            v[i] = v[i+1] * t;
        }

        int x = round(xCoeff * v);
        int y = round(yCoeff * v);
        LineTo(hdc, x, y);

    }

}

void LineClipping(Point p1, Point p2){
    int xLeft = 50, xRight = 300;
    int yTop = 50, yBottom = 300;


    Point topLeft(xLeft, yTop);
    Point bottomLeft(xLeft, yBottom);
    Point topRight(xRight, yTop);
    Point bottomRight(xRight, yBottom);

    draw(topLeft, bottomLeft);
    draw(bottomLeft, bottomRight);
    draw(bottomRight, topRight);
    draw(topRight, topLeft);

    Code code1;
    code1.setCode(p1, xLeft, xRight, yTop ,yBottom);
    Code code2;
    code2.setCode(p2, xLeft, xRight, yTop ,yBottom);

    if (code1.isOut() && code1.isOut()) // both out
    {
        return;
    }

    while(true) // one of them is in
    {

        if (!code1.isOut() && !code2.isOut()) // both in
        {
            draw(p1, p2);
            break;
        }

        if(code1.isOut())
        {

            Point p;

            if(code1.left)
                p = vIntersect(p1, p2, xLeft);
            else if(code1.top)
                p = hIntersect(p1, p2, yTop);
            else if(code1.right)
                p = vIntersect(p1, p2, xRight);
            else
                p = hIntersect(p1, p2, yBottom);

            p1 = p;

            code1.setCode(p1, xLeft, xRight, yTop, yBottom);


        }
        else if (code2.isOut())
        {

            Point p;

            if(code2.left)
                p = vIntersect(p1, p2, xLeft);
            else if(code2.top)
                p = hIntersect(p1, p2, yTop);
            else if(code2.right)
                p = vIntersect(p1, p2, xRight);
            else
                p = hIntersect(p1, p2, yBottom);

            p2 = p;

            code2.setCode(p2, xLeft, xRight, yTop, yBottom);

        }


    }
}


LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;

        case WM_CREATE:
        {
            HMENU HMenuBar = CreateMenu();

            HMENU HFile = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HFile,  "File");
            AppendMenu(HFile, MF_POPUP, 1, "Save");
            AppendMenu(HFile, MF_POPUP, 2, "Load");
            AppendMenu(HFile, MF_POPUP, 7, "Close");

            HMENU HDrawLine = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HDrawLine, "Line");
            AppendMenu(HDrawLine, MF_POPUP, 3, "Bresenham (midpoint)  Algorithm");
            AppendMenu(HDrawLine, MF_POPUP, 4, "Direct Naive Method");
            AppendMenu(HDrawLine, MF_POPUP, 5, "Simple DDA");


            HMENU HDrawCircle = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HDrawCircle, "Circle");
            AppendMenu(HDrawCircle, MF_POPUP, 6, "Cartesian equation");
            AppendMenu(HDrawCircle, MF_POPUP, 8, "Polar equation");
            AppendMenu(HDrawCircle, MF_POPUP, 9, "Bresenham (midpoint) Algorithm");

            HMENU HDrawCurve = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HDrawCurve, "Curve");
            AppendMenu(HDrawCurve, MF_POPUP, 10, "Hermit");
            AppendMenu(HDrawCurve, MF_POPUP, 11, "Bezier");

            HMENU HClipping = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HClipping, "Clipping");
            AppendMenu(HClipping, MF_POPUP, 12, "Line");
            AppendMenu(HClipping, MF_POPUP, 13, "Polygon");

            HMENU HFilling = CreateMenu();
            AppendMenu(HMenuBar, MF_POPUP, (UINT_PTR)HFilling, "Filling");
            AppendMenu(HFilling, MF_POPUP, 14, "Flat");

            SetMenu(hwnd, HMenuBar);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            Point P;
            P.x = LOWORD(lParam);
            P.y = HIWORD(lParam);
            points.push_back(P);
            break;
        }
        case WM_COMMAND:
        {

            command = LOWORD(wParam);
            points.clear();

            break;
        }

        case WM_PAINT:

            hdc = BeginPaint(hwnd, &ps);

            go();

            InvalidateRect(hwnd, NULL,false);
            EndPaint(hwnd, &ps);
            break;

        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}


void go(){
    if (command == 1)
    {
        Save();
        command = -1;
    }
    if (command == 2)
    {
        Load();
        command = -1;
    }
    if(command == 7)
    {
        exit(0);
        command = -1;
    }

    if (points.size() == 5)
    {
        if (command == 14)
        {
            ConvexFill();
            points.clear();
        }
    }

    if (points.size() == 4)
    {
        if (command == 10)
        {
            cout << "Hermit Curve\n";
            HermitCurve(points[0], points[1], points[2], points[3]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 11)
        {
            cout << "Bezier Curve\n";
            BezierCurve(points[0], points[1], points[2], points[3]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
    }

    else if (points.size() == 2)
    {
        if (command == 3)
        {
            cout << "Mid Point Line\n";
            MidPointLine(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 4)
        {
            cout << "Direct Naive Line\n";
            DirectNaiveLine(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 5)
        {
            cout << "Sime DDA Line\n";
            SimpleDDALine(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 6)
        {
            cout << "Catesian Circle\n";
            CartesianCircle(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }

        else if (command == 8)
        {
            cout << "Polar Circle\n";
            PolarCircle(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 9)
        {
            cout << "Mid Point Circle\n";
            MidPointCircle(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
        else if (command == 12)
        {
            cout << "Line Clipping\n";
            LineClipping(points[0], points[1]);
            if (!load)
                SavedShapes.push_back(make_pair(points, command));
            points.clear();
        }
    }
}
