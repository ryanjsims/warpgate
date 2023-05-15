#include <gtkmm.h>
#include <iostream>

static volatile int keep = 0;

class MyWindow : public Gtk::Window {
public:
    MyWindow();
    ~MyWindow() override;

protected:
    void on_button_clicked();

    Gtk::Button m_button;
};

MyWindow::MyWindow() : m_button("Hello world!") {
    set_title("Test app");
    m_button.set_margin(10);
    m_button.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_button_clicked));
    set_child(m_button);
}

MyWindow::~MyWindow() {}

void MyWindow::on_button_clicked() {
    std::cout << "Hello world!" << std::endl;
}

#ifdef _WIN32
int WinMain(void *hInstance, void *hPrevInstance, char **argv, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    #ifdef _WIN32
    int argc = __argc;
    #endif
    std::shared_ptr<Gtk::Application> app = Gtk::Application::create("org.warpgate.hellogtk");
    return app->make_window_and_run<MyWindow>(argc, argv);
}