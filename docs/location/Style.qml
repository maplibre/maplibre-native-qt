/*!
    \class Style
    \brief A QML helper item to be attached to a \c MapView using \c MapLibre.style property.
    \ingroup QMapLibreLocation

    This item does not have any properties and expect to have \ref StyleParameter
    implementations as children. See also \ref SourceParameter and \ref LayerParameter.

    \snippet{trimleft} example_Style.qml Attaching a Style

    Parameters can also be manipulated programatically using \ref addParameter,
    \ref removeParameter and \ref clearParameters functions.

    \snippet{trimleft} snippets_Style.qml Adding a parameter to a style

    \example example_Style.qml
    This is an example of how to use the Style item.
*/

Item {
    /*!
        \brief Add a parameter programatically
        \param type:StyleParameter parameter The parameter to be added
    */
    function addParameter(parameter) {}

    /*!
        \brief Remove a parameter programatically
        \param type:StyleParameter parameter The parameter to be removed
    */
    function removeParameter(parameter) {}

    /*!
        \brief Clear all parameters programatically
    */
    function clearParameters() {}
}
