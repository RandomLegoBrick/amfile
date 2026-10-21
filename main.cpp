
#include <fpdfview.h>
#include <wx/dcbuffer.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/wx.h>
#include <cmath>
#include <iostream>

class DraggableImage : public wxPanel {
public:
    DraggableImage(wxWindow* parent, const wxBitmap &bitmap, const wxPoint &position) 
        : wxPanel(parent, wxID_ANY, position, bitmap.GetSize(), wxBORDER_NONE), image(bitmap) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &DraggableImage::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &DraggableImage::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &DraggableImage::OnLeftUp, this);
		Bind(wxEVT_LEAVE_WINDOW, &DraggableImage::OnLeaveWindow, this);
		Bind(wxEVT_MOTION, &DraggableImage::OnMouseMove, this);
    }   
    
    void changeImage(const wxBitmap &bitmap){
        image = bitmap;
        Refresh();
    }

private:

    bool activeDrag = false;
    wxBitmap image;
    wxPoint dragOffset{0, 0};

    void OnPaint(wxPaintEvent&){
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(wxColour(200, 200, 200)));
        dc.Clear();
        dc.DrawBitmap(image, 0, 0, true);
    }

    void OnLeftDown(wxMouseEvent& evt){
        activeDrag = true;
        dragOffset = evt.GetPosition();
        CaptureMouse();
    }

    void EndDrag() {
        activeDrag = false;
        if(HasCapture()){
            ReleaseMouse();
        }
    }

    void OnLeftUp(wxMouseEvent&) {
        EndDrag();
    }

    void OnLeaveWindow(wxMouseEvent&) {
        if(activeDrag && !HasCapture()){
            EndDrag();
        }
    }

    void OnMouseMove(wxMouseEvent& evt){
        if(!activeDrag || !evt.Dragging() || !evt.LeftIsDown()){
            return; // no need to drag
        }

        if(!HasCapture()){
            CaptureMouse();
        }

        const wxPoint mousePos = wxGetMousePosition();
        const wxPoint windowPos = GetParent()->ScreenToClient(mousePos);
        
        Move(windowPos - dragOffset);
    }

    
};


wxBitmap RenderPdfThumbnail(FPDF_DOCUMENT doc, int pageIndex, int thumbWidth)
{
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return wxBitmap();
    }

    const double pageWidth = FPDF_GetPageWidth(page);
    const double pageHeight = FPDF_GetPageHeight(page);

    const int thumbHeight = std::max(
        1,
        static_cast<int>(std::lround(thumbWidth * pageHeight / pageWidth))
    );

    FPDF_BITMAP bitmap = FPDFBitmap_Create(thumbWidth, thumbHeight, 1);
    if (!bitmap) {
        FPDF_ClosePage(page);
        return wxBitmap();
    }

    FPDFBitmap_FillRect(bitmap, 0, 0, thumbWidth, thumbHeight, 0xFFFFFFFF);

    const int flags = FPDF_ANNOT | FPDF_LCD_TEXT;
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, thumbWidth, thumbHeight, 0, flags);

    unsigned char* src = static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));
    std::vector<unsigned char> rgb(thumbWidth * thumbHeight * 3);

    for (int i = 0; i < thumbWidth * thumbHeight; ++i) {
        rgb[i * 3 + 0] = src[i * 4 + 2];
        rgb[i * 3 + 1] = src[i * 4 + 1];
        rgb[i * 3 + 2] = src[i * 4 + 0];
    }

    wxImage image(thumbWidth, thumbHeight, rgb.data(), true);
    wxBitmap result(image);

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    return result;
}


class MainFrame final : public wxFrame {
public:
    
    int page = 0;
    FPDF_DOCUMENT doc;
    DraggableImage* img;

    MainFrame() : wxFrame(nullptr, wxID_ANY, "AMFILE", wxDefaultPosition, wxSize(800, 600)) {
        auto* panel = new wxPanel(this);
        panel->SetBackgroundColour(wxColour(200, 200, 200));

        doc = FPDF_LoadDocument("../test/test1.pdf", nullptr);
        
        wxBitmap bitmap = RenderPdfThumbnail(doc, 0, 100);


        img = new DraggableImage(panel, bitmap, wxPoint(120, 140));

        auto* button = new wxButton(
            panel,
            wxID_ANY,
            "Next Page",
            wxPoint(20, 20),
            wxSize(120, 35)
        );

        button->Bind(wxEVT_BUTTON, &MainFrame::swapPage, this);

    }

    ~MainFrame() {
        FPDF_CloseDocument(doc);
    }

    void swapPage(wxCommandEvent& evt){
        img->changeImage(RenderPdfThumbnail(doc, ++page, 100));
    }
};

class App final : public wxApp {
public:
    int OnExit() override {
        FPDF_DestroyLibrary();
        
        return wxApp::OnExit();
    }
    bool OnInit() override {
        if(!wxApp::OnInit()){
            return false;
        }
        
        FPDF_InitLibrary();


        auto* frame = new MainFrame();
        frame->Show(true);

        return true;
    }
};

wxIMPLEMENT_APP(App);