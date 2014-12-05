#include "window.h"
#include "widgets.h"
#include "xplat.h"
#include "scene.h"

struct Selection
{
    std::weak_ptr<Object> object;
    ProgramHandle selectionProgram;

    Mesh arrowMesh, circleMesh, scaleMesh;
    ProgramHandle arrowProg;
    std::function<void()> onSelectionChanged;

    Selection();

    void Deselect();
    void SetSelection(std::shared_ptr<Object> object);
};

struct View : public gui::Element
{
    enum Mode { Translation, Rotation, Scaling };

    Scene & scene;
    Selection & selection;
    Pose viewpoint;
    float yaw=0,pitch=0;
    bool bf=0,bl=0,bb=0,br=0;
    Mode mode=Translation;

    const Mesh & GetGizmoMesh() const;

    View(Scene & scene, Selection & selection);

    bool OnKey(GLFWwindow * window, int key, int action, int mods) override;
    void OnUpdate(float timestep);
    void OnDrag(const int2 & delta);

    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;
    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override;
};

class Editor
{
    Window                                  window;
    AssetLibrary                            assets;
    Font                                    font;
    GuiFactory                              factory;
    std::shared_ptr<gui::ListBox>           objectList;
    std::shared_ptr<gui::DockingContainer>  docker;
    gui::ElementPtr                         objectListPanel;
    gui::ElementPtr                         propertyPanel;

    Scene                                   scene;
    Selection                               selection;
    std::shared_ptr<View>                   view;

    bool                                    quit;

    void LoadScene(const std::string & filepath);
    void RefreshMenu();
    void RefreshObjectList();
    void RefreshPropertyPanel();
public:
    Editor();

    int Run();
};