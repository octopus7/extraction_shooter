// Authored by gpt-6-astra; reasoning effort: medium.
#include "UI/Astra/IconByteSpinBoxGPT6Astra.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/ArrangedChildren.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SSpinBox.h"

/** Keep SSpinBox's native editing, selection, caret, focus, keyboard and drag implementation.
 * Only its paint is replaced; each native child is clipped to one side of the fill boundary.
 */
class SIconByteSpinBoxGPT6Astra : public SSpinBox<float>
{
  public:
    TWeakObjectPtr<UIconByteSpinBoxGPT6Astra> Owner;
    virtual int32 OnPaint(const FPaintArgs &Args, const FGeometry &Geo, const FSlateRect &Cull,
                          FSlateWindowElementList &Elements, int32 Layer, const FWidgetStyle &PaintStyle,
                          bool ParentEnabled) const override
    {
        const auto *Input = Owner.Get();
        if (!Input)
            return SSpinBox<float>::OnPaint(Args, Geo, Cull, Elements, Layer, PaintStyle, ParentEnabled);
        const FVector2D Size = Geo.GetLocalSize();
        const float Fraction = FMath::Clamp(GetValue() / 255.0f, 0.0f, 1.0f);
        const double Boundary = Size.X * Fraction;
        const FSlateBrush *Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
        const bool Enabled = ShouldBeEnabled(ParentEnabled);
        const ESlateDrawEffect Effects = Enabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
        FSlateDrawElement::MakeBox(Elements, Layer, Geo.ToPaintGeometry(), Brush, Effects,
                                   Input->UnfilledBackground * PaintStyle.GetColorAndOpacityTint());
        if (Boundary > 0)
            FSlateDrawElement::MakeBox(Elements, Layer + 1,
                                       Geo.ToPaintGeometry(FVector2D(Boundary, Size.Y), FSlateLayoutTransform()), Brush,
                                       Effects, Input->FilledBackground * PaintStyle.GetColorAndOpacityTint());

        FArrangedChildren Children(EVisibility::Visible);
        ArrangeChildren(Geo, Children);
        int32 MaxLayer = Layer + 1;
        auto PaintSide = [&](double X, double Width, const FLinearColor &Foreground) {
            if (Width <= 0)
                return;
            Elements.PushClip(
                FSlateClippingZone(Geo.MakeChild(FVector2D(Width, Size.Y), FSlateLayoutTransform(FVector2D(X, 0)))));
            const FWidgetStyle ChildStyle =
                FWidgetStyle(PaintStyle).BlendColorAndOpacityTint(GetColorAndOpacity()).SetForegroundColor(Foreground);
            for (int32 Index = 0; Index < Children.Num(); ++Index)
            {
                const FArrangedWidget &Child = Children[Index];
                MaxLayer = FMath::Max(MaxLayer, Child.Widget->Paint(Args.WithNewParent(this), Child.Geometry, Cull,
                                                                    Elements, Layer + 2, ChildStyle, Enabled));
            }
            Elements.PopClip();
        };
        PaintSide(0, Boundary, Input->FilledForeground);
        PaintSide(Boundary, Size.X - Boundary, Input->UnfilledForeground);
        return MaxLayer;
    }
#if WITH_DEV_AUTOMATION_TESTS
    void SetEditingText(const FText &Text)
    {
        EnterTextMode();
        // Exercise the actual native editable text, including its selection/caret painting.
        FChildren *Row = GetChildren()->GetChildAt(0)->GetChildren();
        for (int32 I = 0; I < Row->Num(); ++I)
            if (Row->GetChildAt(I)->GetType() == TEXT("SEditableText"))
            {
                auto Edit = StaticCastSharedRef<SEditableText>(Row->GetChildAt(I));
                Edit->SetText(Text);
                FSlateApplication::Get().SetKeyboardFocus(Edit, EFocusCause::SetDirectly);
                Edit->ClearSelection();
            }
    }
    void CommitEditingText(const FText &Text)
    {
        TextField_OnTextCommitted(Text, ETextCommit::OnEnter);
    }
#endif
};

TSharedRef<SWidget> UIconByteSpinBoxGPT6Astra::RebuildWidget()
{
    TSharedRef<SIconByteSpinBoxGPT6Astra> Spin =
        SNew(SIconByteSpinBoxGPT6Astra)
            .Style(&GetWidgetStyle())
            .Font(GetFont())
            .ClearKeyboardFocusOnCommit(GetClearKeyboardFocusOnCommit())
            .SelectAllTextOnCommit(GetSelectAllTextOnCommit())
            .Justification(GetJustification())
            .OnValueChanged(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnValueChanged))
            .OnValueCommitted(BIND_UOBJECT_DELEGATE(FOnFloatValueCommitted, HandleOnValueCommitted))
            .OnBeginSliderMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnBeginSliderMovement))
            .OnEndSliderMovement(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnEndSliderMovement));
    Spin->Owner = this;
    MySpinBox = Spin;
    return Spin;
}
#if WITH_DEV_AUTOMATION_TESTS
void UIconByteSpinBoxGPT6Astra::SetEditingTextForAutomation(const FText &Text)
{
    if (MySpinBox)
        StaticCastSharedPtr<SIconByteSpinBoxGPT6Astra>(MySpinBox)->SetEditingText(Text);
}
void UIconByteSpinBoxGPT6Astra::CommitEditingTextForAutomation(const FText &Text)
{
    if (MySpinBox)
        StaticCastSharedPtr<SIconByteSpinBoxGPT6Astra>(MySpinBox)->CommitEditingText(Text);
}
#endif
